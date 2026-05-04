#pragma once

#include "core/Student.h"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace bdss::utils {

struct TickRecord {
    int time = 0;
    int totalQueueLength = 0;
    int waitingForSeatCount = 0;
    int occupiedSeats = 0;
    int finishedStudents = 0;
    double seatUtilization = 0.0;
};

class StatisticsLogger {
public:
    void recordTick(int time,
                    int totalQueueLength,
                    int waitingForSeatCount,
                    int occupiedSeats,
                    int totalSeats,
                    int finishedStudents);

    void logStudentFinished(const std::shared_ptr<bdss::core::Student>& student);

    void exportCSV(const std::string& filename) const;
    void printSummary(std::ostream& os) const;

    int getFinishedCount() const { return finishedCount_; }
    int getMaxQueueLength() const { return maxQueueLength_; }
    int getMaxWaitingForSeatCount() const { return maxWaitingForSeatCount_; }

    double getAverageWaitTime() const;
    double getAverageSeatWaitTime() const;
    double getAverageServiceTime() const;
    double getAverageTotalTime() const;
    double getAverageSeatUtilization() const;

private:
    std::vector<TickRecord> records_;

    int finishedCount_ = 0;
    int maxQueueLength_ = 0;
    int maxWaitingForSeatCount_ = 0;

    long long totalWaitTime_ = 0;
    long long totalSeatWaitTime_ = 0;
    long long totalServiceTime_ = 0;
    long long totalTime_ = 0;
};

} // namespace bdss::utils