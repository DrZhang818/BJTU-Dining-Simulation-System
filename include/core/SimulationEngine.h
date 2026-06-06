#pragma once

#include "core/Canteen.h"
#include "core/Config.h"
#include "core/Window.h"
#include "utils/StatisticsLogger.h"

#include <deque>
#include <memory>
#include <vector>

namespace bdss::core {

class SimulationEngine {
public:
    explicit SimulationEngine(Config config);

    void reset();
    void tick();
    void run();
    bool isFinished() const noexcept;

    int getCurrentTime() const noexcept { return currentTime_; }
    int getTotalQueueLength() const noexcept;
    int getWaitingForSeatCount() const noexcept { return static_cast<int>(waitingForSeat_.size()); }
    int getOccupiedSeats() const noexcept { return canteen_.getOccupiedSeats(); }
    int getGeneratedStudentCount() const noexcept { return static_cast<int>(allStudents_.size()); }
    int getFinishedStudentCount() const noexcept { return static_cast<int>(finishedStudents_.size()); }

    const Config& config() const noexcept { return config_; }
    const Canteen& canteen() const noexcept { return canteen_; }
    const std::vector<Window>& windows() const noexcept { return windows_; }
    const utils::StatisticsLogger& getStatistics() const noexcept { return statistics_; }

private:
    void generateArrivals();
    Window& chooseWindow();
    Window& chooseWindowWithPreference(const std::shared_ptr<Student>& student);
    std::shared_ptr<Student> createStudentWithPreferences(int serviceTime, int diningTime);
    void assignDiningGroups(int arrivalCount);
    std::vector<double> buildWindowEfficiencies() const;
    bool hasActiveStudents() const noexcept;
    int boundedServiceTimeForWindow(int rawServiceTime, const Window& window) const noexcept;

    Config config_;
    int currentTime_ = 0;
    int nextStudentId_ = 1;
    Canteen canteen_;
    std::vector<Window> windows_;
    std::deque<std::shared_ptr<Student>> waitingForSeat_;
    std::vector<std::shared_ptr<Student>> allStudents_;
    std::vector<std::shared_ptr<Student>> finishedStudents_;
    int droppedStudents_ = 0;
    bool finalized_ = false;
    utils::StatisticsLogger statistics_;
};

} // namespace bdss::core
