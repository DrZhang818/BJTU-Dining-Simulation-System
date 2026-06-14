#pragma once

#include <filesystem>
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

struct Summary {
    int generatedStudents = 0;
    int finishedStudents = 0;
    int droppedStudents = 0;
    int takeawayStudents = 0;
    int maxQueueLength = 0;
    int maxWaitingForSeat = 0;
    int peakQueueTime = 0;
    int peakSeatWaitTime = 0;
    double averageQueueWait = 0.0;
    double averageSeatWait = 0.0;
    double averageSeatUtilization = 0.0;
    double finishRate = 0.0;
    double dropRate = 0.0;
};

class StatisticsLogger {
public:
    void reset();
    void setGeneratedStudents(int value);
    void addQueueWait(double seconds);
    void addSeatWait(double seconds);
    void recordTick(const TickRecord& record);

    const std::vector<TickRecord>& records() const;
    Summary summary() const;
    std::string summaryText() const;
    void exportCsv(const std::filesystem::path& path) const;

private:
    int generatedStudents_ = 0;
    double totalQueueWait_ = 0.0;
    int queueWaitSamples_ = 0;
    double totalSeatWait_ = 0.0;
    int seatWaitSamples_ = 0;
    std::vector<TickRecord> records_;
};

} // namespace bdss::utils
