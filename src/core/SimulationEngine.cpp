#include "core/SimulationEngine.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace bdss::core {

SimulationEngine::SimulationEngine(Config config)
    : config_(std::move(config)), rng_(config_.randomSeed), takeawayDistribution_(config_.takeawayRate) {
    config_.normalize();
    config_.validate();
    windows_.reserve(static_cast<std::size_t>(config_.windowCount));
    for (int i = 0; i < config_.windowCount; ++i) {
        windows_.emplace_back(i);
    }
}

void SimulationEngine::run() {
    statistics_.reset();
    waitingForSeat_.clear();
    diningEndTimes_.clear();
    cleaningEndTimes_.clear();
    nextStudentId_ = 1;
    generatedStudents_ = 0;
    finishedStudents_ = 0;
    droppedStudents_ = 0;
    takeawayStudents_ = 0;

    for (int currentTime = 0; currentTime <= config_.totalSimulationTime; ++currentTime) {
        addArrivals(currentTime);
        processWindows(currentTime);
        processSeats(currentTime);
        recordTick(currentTime);
    }
    statistics_.setGeneratedStudents(generatedStudents_);
}

const Config& SimulationEngine::config() const {
    return config_;
}

const bdss::utils::StatisticsLogger& SimulationEngine::statistics() const {
    return statistics_;
}

bdss::utils::StatisticsLogger& SimulationEngine::statistics() {
    return statistics_;
}

int SimulationEngine::drawServiceDuration() {
    std::normal_distribution<double> dist(config_.avgServiceTime, config_.avgServiceTime * 0.25);
    return std::max(1, static_cast<int>(std::round(dist(rng_))));
}

int SimulationEngine::drawDiningDuration() {
    std::normal_distribution<double> dist(config_.avgDiningTime, config_.avgDiningTime * 0.20);
    return std::max(1, static_cast<int>(std::round(dist(rng_))));
}

int SimulationEngine::drawCleaningDuration() {
    if (config_.avgCleaningTime <= 0.0) {
        return 0;
    }
    std::normal_distribution<double> dist(config_.avgCleaningTime, std::max(1.0, config_.avgCleaningTime * 0.30));
    return std::max(0, static_cast<int>(std::round(dist(rng_))));
}

int SimulationEngine::chooseWindowIndex() const {
    int bestIndex = 0;
    int bestLength = windows_.front().queueLength();
    for (int i = 1; i < static_cast<int>(windows_.size()); ++i) {
        if (windows_[static_cast<std::size_t>(i)].queueLength() < bestLength) {
            bestIndex = i;
            bestLength = windows_[static_cast<std::size_t>(i)].queueLength();
        }
    }
    return bestIndex;
}

void SimulationEngine::addArrivals(int currentTime) {
    const double lambda = config_.arrivalRateAt(currentTime) / 60.0;
    std::poisson_distribution<int> arrivals(lambda);
    const int count = arrivals(rng_);

    for (int i = 0; i < count; ++i) {
        Student student;
        student.id = nextStudentId_++;
        student.arrivalTime = currentTime;
        student.takeaway = takeawayDistribution_(rng_);
        student.state = StudentState::Queuing;
        windows_[static_cast<std::size_t>(chooseWindowIndex())].enqueue(student);
        ++generatedStudents_;
    }
}

void SimulationEngine::processWindows(int currentTime) {
    for (auto& window : windows_) {
        auto result = window.tick(currentTime, drawServiceDuration());
        if (!result.hasStudent) {
            continue;
        }

        auto student = result.student;
        statistics_.addQueueWait(static_cast<double>(std::max(0, student.serviceStartTime - student.arrivalTime)));

        if (student.takeaway) {
            student.state = StudentState::Finished;
            ++finishedStudents_;
            ++takeawayStudents_;
            continue;
        }

        student.seatWaitStartTime = currentTime;
        student.state = StudentState::WaitingForSeat;
        waitingForSeat_.push_back(student);
    }
}

void SimulationEngine::processSeats(int currentTime) {
    auto removeFinished = [currentTime](std::vector<int>& endTimes) {
        endTimes.erase(std::remove_if(endTimes.begin(), endTimes.end(),
                                      [currentTime](int end) { return end <= currentTime; }),
                       endTimes.end());
    };

    const int beforeDining = static_cast<int>(diningEndTimes_.size());
    std::vector<int> endedDining;
    for (int endTime : diningEndTimes_) {
        if (endTime <= currentTime) {
            endedDining.push_back(endTime);
        }
    }
    removeFinished(diningEndTimes_);
    const int releasedSeats = beforeDining - static_cast<int>(diningEndTimes_.size());
    for (int i = 0; i < releasedSeats; ++i) {
        const int cleaningDuration = drawCleaningDuration();
        if (cleaningDuration > 0) {
            cleaningEndTimes_.push_back(currentTime + cleaningDuration);
        }
    }

    removeFinished(cleaningEndTimes_);

    while (!waitingForSeat_.empty()) {
        Student& front = waitingForSeat_.front();
        const int waited = currentTime - front.seatWaitStartTime;
        if (waited <= config_.patienceSeatSeconds) {
            break;
        }
        statistics_.addSeatWait(static_cast<double>(waited));
        ++droppedStudents_;
        waitingForSeat_.pop_front();
    }

    while (!waitingForSeat_.empty()) {
        const int occupiedOrCleaning = static_cast<int>(diningEndTimes_.size() + cleaningEndTimes_.size());
        if (occupiedOrCleaning >= config_.totalSeats) {
            break;
        }

        Student student = waitingForSeat_.front();
        waitingForSeat_.pop_front();
        student.seatStartTime = currentTime;
        student.state = StudentState::Dining;
        statistics_.addSeatWait(static_cast<double>(std::max(0, student.seatStartTime - student.seatWaitStartTime)));
        diningEndTimes_.push_back(currentTime + drawDiningDuration());
        ++finishedStudents_;
    }
}

void SimulationEngine::recordTick(int currentTime) {
    bdss::utils::TickRecord record;
    record.time = currentTime;
    record.waitingForSeatCount = static_cast<int>(waitingForSeat_.size());
    record.occupiedSeats = static_cast<int>(diningEndTimes_.size());
    record.cleaningSeats = static_cast<int>(cleaningEndTimes_.size());
    record.totalSeats = config_.totalSeats;
    record.finishedStudents = finishedStudents_;
    record.droppedStudents = droppedStudents_;
    record.takeawayStudents = takeawayStudents_;

    int totalQueue = 0;
    int servedStudents = 0;
    record.windowQueueLengths.reserve(windows_.size());
    record.windowServedCounts.reserve(windows_.size());
    for (const auto& window : windows_) {
        record.windowQueueLengths.push_back(window.queueLength());
        record.windowServedCounts.push_back(window.servedCount());
        totalQueue += window.queueLength();
        servedStudents += window.servedCount();
    }
    record.totalQueueLength = totalQueue;
    record.seatUtilization = record.totalSeats > 0
        ? static_cast<double>(record.occupiedSeats) / static_cast<double>(record.totalSeats)
        : 0.0;
    record.inSystemStudents = generatedStudents_ - finishedStudents_ - droppedStudents_;

    statistics_.recordTick(record);
}

} // namespace bdss::core
