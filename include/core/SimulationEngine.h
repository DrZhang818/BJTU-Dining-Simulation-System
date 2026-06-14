#pragma once

#include "core/Config.h"
#include "core/Student.h"
#include "core/Window.h"
#include "utils/StatisticsLogger.h"

#include <deque>
#include <random>
#include <utility>
#include <vector>

namespace bdss::core {

class SimulationEngine {
public:
    explicit SimulationEngine(Config config);

    void run();
    const Config& config() const;
    const bdss::utils::StatisticsLogger& statistics() const;
    bdss::utils::StatisticsLogger& statistics();

private:
    int drawServiceDuration();
    int drawDiningDuration();
    int drawCleaningDuration();
    int chooseWindowIndex() const;
    void addArrivals(int currentTime);
    void processWindows(int currentTime);
    void processSeats(int currentTime);
    void recordTick(int currentTime);

    Config config_;
    std::vector<Window> windows_;
    std::deque<Student> waitingForSeat_;
    std::vector<int> diningEndTimes_;
    std::vector<int> cleaningEndTimes_;
    bdss::utils::StatisticsLogger statistics_;
    std::mt19937 rng_;
    std::bernoulli_distribution takeawayDistribution_;
    int nextStudentId_ = 1;
    int generatedStudents_ = 0;
    int finishedStudents_ = 0;
    int droppedStudents_ = 0;
    int takeawayStudents_ = 0;
};

} // namespace bdss::core
