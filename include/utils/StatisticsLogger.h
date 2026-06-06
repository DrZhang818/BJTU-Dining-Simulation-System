#pragma once

#include "core/Student.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace bdss::utils {

struct TickRecord {
    int time = 0;
    int totalQueueLength = 0;
    int waitingForSeatCount = 0;
    int occupiedSeats = 0;
    int cleaningSeats = 0;
    int totalSeats = 0;
    int finishedStudents = 0;
    int droppedStudents = 0;
    int takeawayStudents = 0;
    int inSystemStudents = 0;
    double seatUtilization = 0.0;
    std::vector<int> windowQueueLengths;
    std::vector<int> windowServedCounts;
};

struct SummaryStats {
    int generatedStudents = 0;
    int finishedStudents = 0;
    int droppedStudents = 0;
    int takeawayStudents = 0;
    int maxQueueLength = 0;
    int maxWaitingForSeat = 0;
    double averageQueueWait = 0.0;
    double averageSeatWait = 0.0;
    double averageServiceTime = 0.0;
    double averageDiningTime = 0.0;
    double averageTotalSystemTime = 0.0;
    double averageSeatUtilization = 0.0;
    double preferenceHitRate = 0.0;
};

class StatisticsLogger {
public:
    void recordGenerated(const std::shared_ptr<bdss::core::Student>& student, bool preferenceHit);
    void logStudentFinished(const std::shared_ptr<bdss::core::Student>& student);
    void logStudentDropped(const std::shared_ptr<bdss::core::Student>& student);
    void recordTick(const TickRecord& record);
    void reset();

    const std::vector<TickRecord>& getTickRecords() const { return tickRecords_; }
    SummaryStats getSummary() const;
    int getGeneratedCount() const { return generatedCount_; }
    int getFinishedCount() const { return finishedCount_; }
    int getDroppedCount() const { return droppedCount_; }
    int getTakeawayCount() const { return takeawayCount_; }
    int getPreferenceHitCount() const { return preferenceHitCount_; }

    void exportCsv(const std::filesystem::path& filePath) const;
    std::string summaryText() const;

private:
    int generatedCount_ = 0;
    int finishedCount_ = 0;
    int droppedCount_ = 0;
    int takeawayCount_ = 0;
    int preferenceHitCount_ = 0;

    long long totalQueueWait_ = 0;
    long long totalSeatWait_ = 0;
    long long totalServiceTime_ = 0;
    long long totalDiningTime_ = 0;
    long long totalSystemTime_ = 0;

    std::vector<TickRecord> tickRecords_;
};

} // namespace bdss::utils
