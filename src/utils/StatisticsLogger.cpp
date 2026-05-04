#include "utils/StatisticsLogger.h"

#include <fstream>
#include <iomanip>
#include <iostream>

namespace bdss::utils {

void StatisticsLogger::recordTick(int time,
                                  int totalQueueLength,
                                  int waitingForSeatCount,
                                  int occupiedSeats,
                                  int totalSeats,
                                  int finishedStudents) {
    double seatUtilization = 0.0;
    if (totalSeats > 0) {
        seatUtilization = static_cast<double>(occupiedSeats) / static_cast<double>(totalSeats);
    }

    records_.push_back(TickRecord{
        time,
        totalQueueLength,
        waitingForSeatCount,
        occupiedSeats,
        finishedStudents,
        seatUtilization
    });

    if (totalQueueLength > maxQueueLength_) {
        maxQueueLength_ = totalQueueLength;
    }

    if (waitingForSeatCount > maxWaitingForSeatCount_) {
        maxWaitingForSeatCount_ = waitingForSeatCount;
    }
}

void StatisticsLogger::logStudentFinished(const std::shared_ptr<bdss::core::Student>& student) {
    if (!student) {
        return;
    }

    ++finishedCount_;

    if (student->getWaitTime() >= 0) {
        totalWaitTime_ += student->getWaitTime();
    }

    if (student->getSeatWaitTime() >= 0) {
        totalSeatWaitTime_ += student->getSeatWaitTime();
    }

    if (student->getServiceTime() >= 0) {
        totalServiceTime_ += student->getServiceTime();
    }

    if (student->getTotalTime() >= 0) {
        totalTime_ += student->getTotalTime();
    }
}

void StatisticsLogger::exportCSV(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "[ERROR] 无法写入 CSV 文件: " << filename << '\n';
        return;
    }

    out << "time,total_queue_length,waiting_for_seat_count,occupied_seats,finished_students,seat_utilization\n";

    for (const auto& record : records_) {
        out << record.time << ','
            << record.totalQueueLength << ','
            << record.waitingForSeatCount << ','
            << record.occupiedSeats << ','
            << record.finishedStudents << ','
            << std::fixed << std::setprecision(4) << record.seatUtilization << '\n';
    }
}

void StatisticsLogger::printSummary(std::ostream& os) const {
    os << "\n========== BDSS Simulation Summary ==========" << '\n';
    os << "Finished students: " << getFinishedCount() << '\n';
    os << "Max queue length: " << getMaxQueueLength() << '\n';
    os << "Max waiting-for-seat count: " << getMaxWaitingForSeatCount() << '\n';
    os << "Average queue wait time: " << getAverageWaitTime() << " seconds" << '\n';
    os << "Average seat wait time: " << getAverageSeatWaitTime() << " seconds" << '\n';
    os << "Average service time: " << getAverageServiceTime() << " seconds" << '\n';
    os << "Average total time in canteen: " << getAverageTotalTime() << " seconds" << '\n';
    os << "Average seat utilization: " << getAverageSeatUtilization() * 100.0 << "%" << '\n';
    os << "============================================" << '\n';
}

double StatisticsLogger::getAverageWaitTime() const {
    if (finishedCount_ == 0) {
        return 0.0;
    }
    return static_cast<double>(totalWaitTime_) / static_cast<double>(finishedCount_);
}

double StatisticsLogger::getAverageSeatWaitTime() const {
    if (finishedCount_ == 0) {
        return 0.0;
    }
    return static_cast<double>(totalSeatWaitTime_) / static_cast<double>(finishedCount_);
}

double StatisticsLogger::getAverageServiceTime() const {
    if (finishedCount_ == 0) {
        return 0.0;
    }
    return static_cast<double>(totalServiceTime_) / static_cast<double>(finishedCount_);
}

double StatisticsLogger::getAverageTotalTime() const {
    if (finishedCount_ == 0) {
        return 0.0;
    }
    return static_cast<double>(totalTime_) / static_cast<double>(finishedCount_);
}

double StatisticsLogger::getAverageSeatUtilization() const {
    if (records_.empty()) {
        return 0.0;
    }

    double sum = 0.0;
    for (const auto& record : records_) {
        sum += record.seatUtilization;
    }

    return sum / static_cast<double>(records_.size());
}

} // namespace bdss::utils