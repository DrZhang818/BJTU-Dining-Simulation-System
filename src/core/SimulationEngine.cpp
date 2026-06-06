#include "core/SimulationEngine.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace bdss::core {
namespace {

WindowProfile makeDefaultWindowProfile(int index, const Config& config) {
    WindowProfile profile;
    profile.name = "窗口 " + std::to_string(index + 1);
    profile.category = config.windowCategories[static_cast<std::size_t>(index) % config.windowCategories.size()];
    switch (config.windowEfficiency) {
        case WindowEfficiency::Uniform:
            profile.efficiency = 1.0;
            break;
        case WindowEfficiency::Variable:
            profile.efficiency = 0.85 + 0.1 * static_cast<double>(index % 5);
            break;
        case WindowEfficiency::Custom:
            profile.efficiency = 1.0;
            break;
    }
    return profile;
}

int clampInt(int value, int lo, int hi) {
    return std::max(lo, std::min(hi, value));
}

} // namespace

SimulationEngine::SimulationEngine(const Config& config)
    : config_(config), canteen_(config.tableRows, config.tableCols), rng_(config.randomSeed) {
    config_.normalize();
    config_.validate();
    setupWindows();
}

WindowProfile SimulationEngine::profileForWindow(int index, const Config& config) const {
    WindowProfile profile;
    if (config.windowEfficiency == WindowEfficiency::Custom && index < static_cast<int>(config.windowProfiles.size())) {
        profile = config.windowProfiles[static_cast<std::size_t>(index)];
        if (profile.name.empty()) {
            profile.name = "窗口 " + std::to_string(index + 1);
        }
        if (profile.category.empty()) {
            profile.category = config.windowCategories[static_cast<std::size_t>(index) % config.windowCategories.size()];
        }
        if (profile.efficiency <= 0.0) {
            profile.efficiency = 1.0;
        }
    } else {
        profile = makeDefaultWindowProfile(index, config);
    }
    return profile;
}

void SimulationEngine::setupWindows() {
    windows_.clear();
    windows_.reserve(static_cast<std::size_t>(config_.windowCount));
    for (int i = 0; i < config_.windowCount; ++i) {
        const WindowProfile profile = profileForWindow(i, config_);
        windows_.emplace_back(i + 1, profile.name, profile.category, profile.efficiency);
    }
}

void SimulationEngine::updateConfig(const Config& config, bool preserveRuntimeState) {
    Config updated = config;
    updated.normalize();
    updated.validate();

    if (!preserveRuntimeState) {
        config_ = updated;
        canteen_ = Canteen(config_.tableRows, config_.tableCols);
        std::queue<std::shared_ptr<Student>> emptyQueue;
        waitingForSeat_.swap(emptyQueue);
        logger_.reset();
        rng_.seed(config_.randomSeed);
        currentTime_ = 0;
        nextStudentId_ = 1;
        nextGroupId_ = 1;
        setupWindows();
        return;
    }

    config_ = updated;
    canteen_.resize(config_.tableRows, config_.tableCols);
    rebuildWindowsForConfig(config_, true);
}

void SimulationEngine::rebuildWindowsForConfig(const Config& config, bool preserveRuntimeState) {
    if (!preserveRuntimeState) {
        setupWindows();
        return;
    }

    std::vector<Window> previous = std::move(windows_);
    std::vector<std::shared_ptr<Student>> displaced;
    std::vector<Window> rebuilt;
    rebuilt.reserve(static_cast<std::size_t>(config.windowCount));

    for (int i = 0; i < config.windowCount; ++i) {
        const WindowProfile profile = profileForWindow(i, config);
        if (i < static_cast<int>(previous.size())) {
            Window window = std::move(previous[static_cast<std::size_t>(i)]);
            window.setProfile(profile.name, profile.category, profile.efficiency);
            rebuilt.push_back(std::move(window));
        } else {
            rebuilt.emplace_back(i + 1, profile.name, profile.category, profile.efficiency);
        }
    }

    for (int i = config.windowCount; i < static_cast<int>(previous.size()); ++i) {
        if (auto current = previous[static_cast<std::size_t>(i)].releaseCurrentStudent()) {
            displaced.push_back(current);
        }
        auto queued = previous[static_cast<std::size_t>(i)].releaseQueuedStudents();
        displaced.insert(displaced.end(), queued.begin(), queued.end());
    }

    windows_ = std::move(rebuilt);
    for (const auto& student : displaced) {
        if (!student || windows_.empty()) {
            continue;
        }
        const int target = chooseWindowIndex(*student);
        windows_[static_cast<std::size_t>(target)].enqueue(student, currentTime_);
    }
}

void SimulationEngine::tick() {
    if (isFinished()) {
        return;
    }

    if (currentTime_ < config_.totalSimulationTime) {
        generateStudents();
    }

    handlePatienceAndQueueSwitching();

    for (auto& window : windows_) {
        auto completedStudent = window.tick(currentTime_);
        if (!completedStudent) {
            continue;
        }
        if (completedStudent->isTakeaway()) {
            logger_.logStudentFinished(completedStudent);
        } else {
            completedStudent->startWaitingForSeat(currentTime_ + 1);
            waitingForSeat_.push(completedStudent);
        }
    }

    auto finishedStudents = canteen_.tick(currentTime_ + 1, config_);
    for (const auto& student : finishedStudents) {
        logger_.logStudentFinished(student);
    }

    seatWaitingStudents(currentTime_ + 1);

    bdss::utils::TickRecord record;
    record.time = currentTime_ + 1;
    record.totalQueueLength = getTotalQueueLength();
    record.waitingForSeatCount = getWaitingForSeatCount();
    record.occupiedSeats = canteen_.getOccupiedSeats();
    record.cleaningSeats = canteen_.getCleaningSeats();
    record.totalSeats = canteen_.getTotalSeats();
    record.finishedStudents = logger_.getFinishedCount();
    record.droppedStudents = logger_.getDroppedCount();
    record.takeawayStudents = logger_.getTakeawayCount();
    record.inSystemStudents = getInSystemCount();
    record.seatUtilization = canteen_.getSeatUtilization();
    record.windowQueueLengths = getWindowQueueLengths();
    record.windowServedCounts = getWindowServedCounts();
    logger_.recordTick(record);

    ++currentTime_;
}

void SimulationEngine::run() {
    while (!isFinished()) {
        tick();
    }
}

bool SimulationEngine::isFinished() const {
    return currentTime_ >= config_.totalSimulationTime && isSystemEmpty();
}

int SimulationEngine::getTotalQueueLength() const {
    int total = 0;
    for (const auto& window : windows_) {
        total += static_cast<int>(window.getQueueLength());
    }
    return total;
}

int SimulationEngine::getInSystemCount() const {
    int total = getTotalQueueLength() + getWaitingForSeatCount() + canteen_.getOccupiedSeats();
    for (const auto& window : windows_) {
        if (window.hasCurrentStudent()) {
            ++total;
        }
    }
    return total;
}

std::vector<int> SimulationEngine::getWindowQueueLengths() const {
    std::vector<int> result;
    result.reserve(windows_.size());
    for (const auto& window : windows_) {
        result.push_back(static_cast<int>(window.getQueueLength()));
    }
    return result;
}

std::vector<int> SimulationEngine::getWindowServedCounts() const {
    std::vector<int> result;
    result.reserve(windows_.size());
    for (const auto& window : windows_) {
        result.push_back(window.getServedCount());
    }
    return result;
}

std::vector<double> SimulationEngine::getWindowEfficiencies() const {
    std::vector<double> result;
    result.reserve(windows_.size());
    for (const auto& window : windows_) {
        result.push_back(window.getEfficiency());
    }
    return result;
}

std::vector<WindowSnapshot> SimulationEngine::getWindowSnapshots() const {
    std::vector<WindowSnapshot> result;
    result.reserve(windows_.size());
    for (const auto& window : windows_) {
        WindowSnapshot snapshot;
        snapshot.id = window.getId();
        snapshot.name = window.getName();
        snapshot.category = window.getCategory();
        snapshot.efficiency = window.getEfficiency();
        snapshot.queueLength = static_cast<int>(window.getQueueLength());
        snapshot.servedCount = window.getServedCount();
        snapshot.serving = window.hasCurrentStudent();
        snapshot.remainingServiceTime = window.getRemainingServiceTime();
        if (auto current = window.getCurrentStudent()) {
            snapshot.currentStudentId = current->getId();
        }
        result.push_back(std::move(snapshot));
    }
    return result;
}

void SimulationEngine::generateStudents() {
    std::poisson_distribution<int> poisson(currentArrivalLambdaPerSecond());
    int arrivals = poisson(rng_);
    int generated = 0;
    while (generated < arrivals) {
        int groupSize = 1;
        if (config_.enableGroupDining && arrivals - generated >= 2 && bernoulli(config_.groupProbability)) {
            groupSize = std::min(sampleGroupSize(), arrivals - generated);
        }
        generateStudentGroup(groupSize);
        generated += groupSize;
    }
}

void SimulationEngine::generateStudentGroup(int groupSize) {
    const int groupId = groupSize > 1 ? nextGroupId_++ : 0;
    const std::string sharedPreference = samplePreferredCategory();
    for (int i = 0; i < groupSize; ++i) {
        const StudentTypeProfile type = sampleStudentType();
        const bool takeaway = config_.enableTakeaway && bernoulli(config_.takeawayRate);
        int serviceTime = samplePositiveNormal(config_.avgServiceTime, config_.serviceStddev);
        if (takeaway) {
            serviceTime = std::max(1, static_cast<int>(std::ceil(static_cast<double>(serviceTime) * config_.packingServiceFactor)));
        }
        const int diningTime = samplePositiveNormal(config_.avgDiningTime * type.diningTimeFactor, config_.diningStddev);
        const int patience = samplePositiveNormal(config_.avgPatienceTime * type.patienceFactor, config_.patienceStddev);
        const std::string preference = config_.enableWindowPreferences ? sharedPreference : std::string();

        auto student = std::make_shared<Student>(nextStudentId_++,
                                                 currentTime_,
                                                 serviceTime,
                                                 diningTime,
                                                 type.name,
                                                 preference,
                                                 takeaway,
                                                 patience,
                                                 groupId,
                                                 groupSize);
        const int windowIndex = chooseWindowIndex(*student);
        const bool preferenceHit = config_.enableWindowPreferences &&
                                   !student->getPreferredCategory().empty() &&
                                   windows_[static_cast<std::size_t>(windowIndex)].getCategory() == student->getPreferredCategory();
        windows_[static_cast<std::size_t>(windowIndex)].enqueue(student, currentTime_);
        logger_.recordGenerated(student, preferenceHit);
    }
}

int SimulationEngine::chooseWindowIndex(const Student& student) const {
    if (windows_.empty()) {
        throw std::runtime_error("No service windows configured");
    }

    int bestIndex = 0;
    double bestScore = std::numeric_limits<double>::infinity();
    int bestQueue = std::numeric_limits<int>::max();
    int bestServed = std::numeric_limits<int>::max();

    for (int i = 0; i < static_cast<int>(windows_.size()); ++i) {
        const auto& window = windows_[static_cast<std::size_t>(i)];
        const int queueLength = static_cast<int>(window.getQueueLength()) + (window.hasCurrentStudent() ? 1 : 0);
        double score = config_.queueWeight * static_cast<double>(queueLength) / std::max(0.05, window.getEfficiency());
        score -= config_.efficiencyWeight * std::log(std::max(0.05, window.getEfficiency()));
        if (config_.enableWindowPreferences && !student.getPreferredCategory().empty() &&
            window.getCategory() == student.getPreferredCategory()) {
            score -= config_.preferenceBonus;
        }

        if (score < bestScore - 1e-9 ||
            (std::abs(score - bestScore) <= 1e-9 && queueLength < bestQueue) ||
            (std::abs(score - bestScore) <= 1e-9 && queueLength == bestQueue && window.getServedCount() < bestServed)) {
            bestScore = score;
            bestIndex = i;
            bestQueue = queueLength;
            bestServed = window.getServedCount();
        }
    }
    return bestIndex;
}

void SimulationEngine::handlePatienceAndQueueSwitching() {
    if (!config_.enablePatience) {
        return;
    }

    std::vector<std::shared_ptr<Student>> impatient;
    for (auto& window : windows_) {
        auto extracted = window.removeExpiredQueuedStudents(currentTime_);
        impatient.insert(impatient.end(), extracted.begin(), extracted.end());
    }

    for (const auto& student : impatient) {
        if (!student) {
            continue;
        }
        if (bernoulli(config_.queueSwitchProbability)) {
            const int newIndex = chooseWindowIndex(*student);
            windows_[static_cast<std::size_t>(newIndex)].enqueue(student, currentTime_);
        } else {
            student->drop(currentTime_);
            logger_.logStudentDropped(student);
        }
    }
}

void SimulationEngine::seatWaitingStudents(int currentTime) {
    std::queue<std::shared_ptr<Student>> remaining;
    while (!waitingForSeat_.empty()) {
        auto student = waitingForSeat_.front();
        waitingForSeat_.pop();
        if (!canteen_.trySeat(student, currentTime, config_)) {
            remaining.push(student);
            break;
        }
    }
    while (!waitingForSeat_.empty()) {
        remaining.push(waitingForSeat_.front());
        waitingForSeat_.pop();
    }
    waitingForSeat_ = std::move(remaining);
}

bool SimulationEngine::isSystemEmpty() const {
    for (const auto& window : windows_) {
        if (!window.empty()) {
            return false;
        }
    }
    return waitingForSeat_.empty() && canteen_.getOccupiedSeats() == 0 && canteen_.getCleaningSeats() == 0;
}

double SimulationEngine::currentArrivalLambdaPerSecond() const {
    double peoplePerMinute = config_.arrivalRate;
    if (config_.arrivalPattern == ArrivalPattern::RushPeaks) {
        for (const auto& peak : config_.rushPeaks) {
            if (currentTime_ >= peak.start && currentTime_ < peak.end) {
                peoplePerMinute *= peak.multiplier;
            }
        }
    }
    return std::max(0.0, peoplePerMinute / 60.0);
}

StudentTypeProfile SimulationEngine::sampleStudentType() {
    double sum = 0.0;
    for (const auto& type : config_.studentTypes) {
        sum += std::max(0.0, type.ratio);
    }
    if (sum <= 0.0) {
        return config_.studentTypes.front();
    }
    std::uniform_real_distribution<double> dist(0.0, sum);
    double value = dist(rng_);
    for (const auto& type : config_.studentTypes) {
        value -= std::max(0.0, type.ratio);
        if (value <= 0.0) {
            return type;
        }
    }
    return config_.studentTypes.back();
}

std::string SimulationEngine::samplePreferredCategory() {
    if (config_.windowCategories.empty()) {
        return {};
    }
    std::uniform_int_distribution<int> dist(0, static_cast<int>(config_.windowCategories.size()) - 1);
    return config_.windowCategories[static_cast<std::size_t>(dist(rng_))];
}

int SimulationEngine::samplePositiveNormal(double mean, double stddev, int minValue) {
    if (stddev <= 0.0) {
        return std::max(minValue, static_cast<int>(std::round(mean)));
    }
    std::normal_distribution<double> dist(mean, stddev);
    return std::max(minValue, static_cast<int>(std::round(dist(rng_))));
}

bool SimulationEngine::bernoulli(double probability) {
    probability = std::clamp(probability, 0.0, 1.0);
    std::bernoulli_distribution dist(probability);
    return dist(rng_);
}

int SimulationEngine::sampleGroupSize() {
    const int hi = clampInt(config_.maxGroupSize, 2, 12);
    std::uniform_int_distribution<int> dist(2, hi);
    return dist(rng_);
}

} // namespace bdss::core
