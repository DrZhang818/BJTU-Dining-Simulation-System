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

    int getCurrentTime() const noexcept;
    int getTotalQueueLength() const noexcept;
    int getWaitingForSeatCount() const noexcept;
    int getOccupiedSeats() const noexcept;
    int getGeneratedStudentCount() const noexcept;

    const Config& config() const noexcept;
    const Canteen& canteen() const noexcept;
    const std::vector<Window>& windows() const noexcept;
    const utils::StatisticsLogger& getStatistics() const noexcept;

private:
    void generateArrivals();
    Window& chooseWindow();
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
    utils::StatisticsLogger statistics_;
};

} // namespace bdss::core
