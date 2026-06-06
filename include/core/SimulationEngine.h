#pragma once

#include "core/Canteen.h"
#include "core/Config.h"
#include "core/Window.h"
#include "utils/StatisticsLogger.h"

#include <memory>
#include <queue>
#include <random>
#include <string>
#include <vector>

namespace bdss::core {

struct WindowSnapshot {
    int id = 0;
    std::string name;
    std::string category;
    double efficiency = 1.0;
    int queueLength = 0;
    int servedCount = 0;
    bool serving = false;
    int currentStudentId = -1;
    int remainingServiceTime = 0;
};

class SimulationEngine {
public:
    explicit SimulationEngine(const Config& config);

    void tick();
    void run();
    void updateConfig(const Config& config, bool preserveRuntimeState = true);
    bool isFinished() const;

    const Config& getConfig() const { return config_; }
    int getCurrentTime() const { return currentTime_; }
    int getTotalSimulationTime() const { return config_.totalSimulationTime; }
    int getGeneratedCount() const { return logger_.getGeneratedCount(); }
    int getDroppedCount() const { return logger_.getDroppedCount(); }
    int getTakeawayCount() const { return logger_.getTakeawayCount(); }
    int getTotalQueueLength() const;
    int getWaitingForSeatCount() const { return static_cast<int>(waitingForSeat_.size()); }
    int getOccupiedSeats() const { return canteen_.getOccupiedSeats(); }
    int getCleaningSeats() const { return canteen_.getCleaningSeats(); }
    int getTotalSeats() const { return canteen_.getTotalSeats(); }
    double getSeatUtilization() const { return canteen_.getSeatUtilization(); }
    int getInSystemCount() const;

    std::vector<int> getWindowQueueLengths() const;
    std::vector<int> getWindowServedCounts() const;
    std::vector<double> getWindowEfficiencies() const;
    std::vector<WindowSnapshot> getWindowSnapshots() const;
    std::vector<SeatSnapshot> getSeatSnapshots() const { return canteen_.getSeatSnapshots(); }
    const bdss::utils::StatisticsLogger& getStatistics() const { return logger_; }

private:
    void setupWindows();
    WindowProfile profileForWindow(int index, const Config& config) const;
    void rebuildWindowsForConfig(const Config& config, bool preserveRuntimeState);
    void generateStudents();
    void generateStudentGroup(int groupSize);
    int chooseWindowIndex(const Student& student) const;
    void handlePatienceAndQueueSwitching();
    void seatWaitingStudents(int currentTime);
    bool isSystemEmpty() const;
    double currentArrivalLambdaPerSecond() const;
    StudentTypeProfile sampleStudentType();
    std::string samplePreferredCategory();
    int samplePositiveNormal(double mean, double stddev, int minValue = 1);
    bool bernoulli(double probability);
    int sampleGroupSize();

    Config config_;
    Canteen canteen_;
    std::vector<Window> windows_;
    std::queue<std::shared_ptr<Student>> waitingForSeat_;
    bdss::utils::StatisticsLogger logger_;
    std::mt19937 rng_;
    int currentTime_ = 0;
    int nextStudentId_ = 1;
    int nextGroupId_ = 1;
};

} // namespace bdss::core
