#include "core/SimulationEngine.h"

#include "utils/RandomGenerator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace bdss::core {

SimulationEngine::SimulationEngine(Config config)
    : config_(config), canteen_(config.tableRows, config.tableCols) {
    config_.validate();
    reset();
}

void SimulationEngine::reset() {
    config_.validate();
    currentTime_ = 0;
    nextStudentId_ = 1;
    canteen_ = Canteen(config_.tableRows, config_.tableCols);
    windows_.clear();
    const auto efficiencies = buildWindowEfficiencies();
    for (int i = 0; i < config_.windowCount; ++i) {
        windows_.emplace_back(i + 1, efficiencies.at(static_cast<std::size_t>(i)));
    }
    waitingForSeat_.clear();
    allStudents_.clear();
    finishedStudents_.clear();
    statistics_.clear();
    utils::RandomGenerator::setSeed(config_.randomSeed);
}

std::vector<double> SimulationEngine::buildWindowEfficiencies() const {
    std::vector<double> efficiencies;
    efficiencies.reserve(static_cast<std::size_t>(config_.windowCount));
    for (int i = 0; i < config_.windowCount; ++i) {
        if (config_.windowEfficiency == WindowEfficiency::Equal) {
            efficiencies.push_back(1.0);
        } else {
            // Deterministic variance around 1.0 avoids flaky tests and makes
            // capacity differences visible in simulation results.
            static constexpr double pattern[] = {0.85, 0.95, 1.05, 1.15, 1.00};
            efficiencies.push_back(pattern[static_cast<std::size_t>(i % 5)]);
        }
    }
    return efficiencies;
}

int SimulationEngine::boundedServiceTimeForWindow(int rawServiceTime, const Window& window) const noexcept {
    const auto adjusted = static_cast<int>(std::ceil(static_cast<double>(rawServiceTime) / window.getEfficiency()));
    return std::max(1, adjusted);
}

Window& SimulationEngine::chooseWindow() {
    return *std::min_element(windows_.begin(), windows_.end(), [](const Window& lhs, const Window& rhs) {
        if (lhs.getWorkloadLength() == rhs.getWorkloadLength()) {
            return lhs.getId() < rhs.getId();
        }
        return lhs.getWorkloadLength() < rhs.getWorkloadLength();
    });
}

void SimulationEngine::generateArrivals() {
    if (currentTime_ >= config_.totalSimulationTime) {
        return;
    }
    const int arrivals = utils::RandomGenerator::getPoisson(config_.arrivalRatePerSecond(currentTime_));
    for (int i = 0; i < arrivals; ++i) {
        auto serviceTime = utils::RandomGenerator::getNormalInt(config_.avgServiceTime, config_.serviceStddev, 1);
        auto diningTime = utils::RandomGenerator::getNormalInt(config_.avgDiningTime, config_.diningStddev, 1);
        auto student = std::make_shared<Student>(nextStudentId_++, currentTime_, serviceTime, diningTime);
        Window& window = chooseWindow();
        student->setServiceTime(boundedServiceTimeForWindow(serviceTime, window));
        window.enqueue(student, currentTime_);
        allStudents_.push_back(student);
    }
}

void SimulationEngine::tick() {
    generateArrivals();

    for (auto& window : windows_) {
        auto completed = window.tick(currentTime_);
        if (completed) {
            waitingForSeat_.push_back(completed);
        }
    }

    auto newlyFinished = canteen_.tick(currentTime_);
    for (auto& student : newlyFinished) {
        finishedStudents_.push_back(student);
    }

    while (!waitingForSeat_.empty() && canteen_.hasEmptySeat()) {
        auto student = waitingForSeat_.front();
        if (!canteen_.trySeat(student, currentTime_ + 1)) {
            break;
        }
        waitingForSeat_.pop_front();
    }

    statistics_.record(currentTime_ + 1,
                       getTotalQueueLength(),
                       getWaitingForSeatCount(),
                       getOccupiedSeats(),
                       static_cast<int>(finishedStudents_.size()),
                       canteen_.utilization());

    ++currentTime_;
}

void SimulationEngine::run() {
    const int safetyLimit = config_.totalSimulationTime + std::max(3600, config_.avgDiningTime * 6 + config_.avgServiceTime * config_.windowCount * 20);
    while (!isFinished()) {
        if (currentTime_ > safetyLimit) {
            throw std::runtime_error("simulation safety limit reached; check configuration capacity and arrival rate");
        }
        tick();
    }
    statistics_.finalize(finishedStudents_);
}

bool SimulationEngine::hasActiveStudents() const noexcept {
    for (const auto& window : windows_) {
        if (!window.empty()) {
            return true;
        }
    }
    return !waitingForSeat_.empty() || canteen_.getOccupiedSeats() > 0;
}

bool SimulationEngine::isFinished() const noexcept {
    return currentTime_ >= config_.totalSimulationTime && !hasActiveStudents();
}

int SimulationEngine::getCurrentTime() const noexcept {
    return currentTime_;
}

int SimulationEngine::getTotalQueueLength() const noexcept {
    int total = 0;
    for (const auto& window : windows_) {
        total += static_cast<int>(window.getQueueLength());
    }
    return total;
}

int SimulationEngine::getWaitingForSeatCount() const noexcept {
    return static_cast<int>(waitingForSeat_.size());
}

int SimulationEngine::getOccupiedSeats() const noexcept {
    return canteen_.getOccupiedSeats();
}

int SimulationEngine::getGeneratedStudentCount() const noexcept {
    return static_cast<int>(allStudents_.size());
}

const Config& SimulationEngine::config() const noexcept {
    return config_;
}

const Canteen& SimulationEngine::canteen() const noexcept {
    return canteen_;
}

const std::vector<Window>& SimulationEngine::windows() const noexcept {
    return windows_;
}

const utils::StatisticsLogger& SimulationEngine::getStatistics() const noexcept {
    return statistics_;
}

} // namespace bdss::core
