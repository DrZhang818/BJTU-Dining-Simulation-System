#pragma once

#include "core/Canteen.h"
#include "core/Config.h"
#include "core/Window.h"
#include "utils/StatisticsLogger.h"

#include <memory>
#include <queue>
#include <vector>

namespace bdss::core {

class SimulationEngine {
public:
    explicit SimulationEngine(const Config& config);

    void tick();
    void run();

    bool isFinished() const;

    int getCurrentTime() const { return currentTime_; }
    int getTotalQueueLength() const;
    int getWaitingForSeatCount() const { return static_cast<int>(waitingForSeat_.size()); }
    int getOccupiedSeats() const { return canteen_.getOccupiedSeats(); }

    std::vector<int> getWindowQueueLengths() const;

    int getTotalSimulationTime() const { return config_.totalSimulationTime; }
    int getTotalSeats() const { return canteen_.getTotalSeats(); }
    double getSeatUtilization() const { return canteen_.getSeatUtilization(); }

    const Canteen& getCanteen() const { return canteen_; }

    std::vector<double> getWindowEfficiencies() const;

    const bdss::utils::StatisticsLogger& getStatistics() const { return logger_; }

private:
    void generateStudents();
    int chooseWindowIndex() const;
    void seatWaitingStudents(int currentTime);
    bool isSystemEmpty() const;
    double currentArrivalLambdaPerSecond() const;

private:
    Config config_;
    int currentTime_ = 0;
    int nextStudentId_ = 1;

    std::vector<Window> windows_;
    Canteen canteen_;
    std::queue<std::shared_ptr<Student>> waitingForSeat_;

    bdss::utils::StatisticsLogger logger_;
};

} // namespace bdss::core