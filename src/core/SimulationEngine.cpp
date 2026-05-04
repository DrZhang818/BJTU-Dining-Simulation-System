#include "core/SimulationEngine.h"

#include "utils/RandomGenerator.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace bdss::core {

SimulationEngine::SimulationEngine(const Config& config)
    : config_(config), canteen_(config.tableRows, config.tableCols) {
    if (config_.windowCount <= 0) {
        throw std::invalid_argument("windowCount must be greater than 0");
    }

    if (config_.tableRows <= 0 || config_.tableCols <= 0) {
        throw std::invalid_argument("tableRows and tableCols must be greater than 0");
    }

    if (config_.totalSimulationTime <= 0) {
        throw std::invalid_argument("totalSimulationTime must be greater than 0");
    }

    bdss::utils::RandomGenerator::setSeed(config_.randomSeed);

    windows_.reserve(config_.windowCount);
    for (int i = 0; i < config_.windowCount; ++i) {
        double efficiency = 1.0;
        if (config_.windowEfficiency == WindowEfficiency::Variable) {
            // 简单模拟窗口效率差异：0.8, 0.9, 1.0, 1.1, 1.2 循环
            efficiency = 0.8 + 0.1 * static_cast<double>(i % 5);
        }
        windows_.emplace_back(i + 1, efficiency);
    }
}

void SimulationEngine::tick() {
    if (isFinished()) {
        return;
    }

    if (currentTime_ < config_.totalSimulationTime) {
        generateStudents();
    }

    for (auto& window : windows_) {
        auto completedStudent = window.tick(currentTime_);
        if (completedStudent) {
            waitingForSeat_.push(completedStudent);
        }
    }

    auto finishedStudents = canteen_.tick(currentTime_);
    for (const auto& student : finishedStudents) {
        logger_.logStudentFinished(student);
    }

    seatWaitingStudents(currentTime_ + 1);

    logger_.recordTick(currentTime_ + 1,
                       getTotalQueueLength(),
                       getWaitingForSeatCount(),
                       canteen_.getOccupiedSeats(),
                       canteen_.getTotalSeats(),
                       logger_.getFinishedCount());

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

std::vector<int> SimulationEngine::getWindowQueueLengths() const {
    std::vector<int> result;
    result.reserve(windows_.size());

    for (const auto& window : windows_) {
        result.push_back(static_cast<int>(window.getQueueLength()));
    }

    return result;
}

void SimulationEngine::generateStudents() {
    const int newStudents = bdss::utils::RandomGenerator::getPoisson(currentArrivalLambdaPerSecond());

    for (int i = 0; i < newStudents; ++i) {
        const int serviceTime = std::max(1, static_cast<int>(std::round(
            bdss::utils::RandomGenerator::getNormal(config_.avgServiceTime, config_.serviceStddev))));

        const int diningTime = std::max(1, static_cast<int>(std::round(
            bdss::utils::RandomGenerator::getNormal(config_.avgDiningTime, config_.diningStddev))));

        auto student = std::make_shared<Student>(nextStudentId_++, currentTime_, serviceTime, diningTime);
        const int windowIndex = chooseWindowIndex();
        windows_[windowIndex].enqueue(student, currentTime_);
    }
}

int SimulationEngine::chooseWindowIndex() const {
    int bestIndex = 0;
    std::size_t bestLength = windows_[0].getQueueLength();

    for (int i = 1; i < static_cast<int>(windows_.size()); ++i) {
        const auto length = windows_[i].getQueueLength();
        if (length < bestLength) {
            bestLength = length;
            bestIndex = i;
        }
    }

    return bestIndex;
}

void SimulationEngine::seatWaitingStudents(int currentTime) {
    std::queue<std::shared_ptr<Student>> remaining;

    while (!waitingForSeat_.empty()) {
        auto student = waitingForSeat_.front();
        waitingForSeat_.pop();

        if (!canteen_.trySeat(student, currentTime)) {
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

    return waitingForSeat_.empty() && canteen_.getOccupiedSeats() == 0;
}

double SimulationEngine::currentArrivalLambdaPerSecond() const {
    double peoplePerMinute = config_.arrivalRate;

    if (config_.arrivalPattern == ArrivalPattern::RushHour &&
        currentTime_ >= config_.rushHourStart &&
        currentTime_ <= config_.rushHourEnd) {
        peoplePerMinute *= config_.rushHourMultiplier;
    }

    return peoplePerMinute / 60.0;
}

} // namespace bdss::core