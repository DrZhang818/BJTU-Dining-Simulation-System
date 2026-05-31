#pragma once

#include "core/Student.h"

#include <filesystem>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace bdss::utils {

struct Snapshot {
    int time = 0;
    int totalQueueLength = 0;
    int waitingForSeatCount = 0;
    int occupiedSeats = 0;
    int finishedStudents = 0;
    double seatUtilization = 0.0;
};

struct SimulationSummary {
    int finishedStudents = 0;
    int maxQueueLength = 0;
    int maxWaitingForSeatCount = 0;
    double averageQueueWaitTime = 0.0;
    double averageSeatWaitTime = 0.0;
    double averageServiceTime = 0.0;
    double averageTotalTimeInCanteen = 0.0;
    double averageSeatUtilization = 0.0;
};

class StatisticsLogger {
public:
    void clear();
    void record(int time,
                int totalQueueLength,
                int waitingForSeatCount,
                int occupiedSeats,
                int finishedStudents,
                double seatUtilization);
    void finalize(const std::vector<std::shared_ptr<bdss::core::Student>>& finishedStudents);

    const SimulationSummary& summary() const noexcept;
    const std::vector<Snapshot>& snapshots() const noexcept;
    int getFinishedCount() const noexcept;

    void exportCSV(const std::filesystem::path& outputPath) const;
    void printSummary(std::ostream& os) const;
    std::string summaryText() const;

private:
    std::vector<Snapshot> snapshots_;
    SimulationSummary summary_;
};

} // namespace bdss::utils
