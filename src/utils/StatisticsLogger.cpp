#include "utils/StatisticsLogger.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace bdss::utils {
namespace {

std::string joinInts(const std::vector<int>& values) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            oss << ';';
        }
        oss << values[i];
    }
    return oss.str();
}

double safeAverage(long long total, int count) {
    return count <= 0 ? 0.0 : static_cast<double>(total) / static_cast<double>(count);
}

} // namespace

void StatisticsLogger::recordGenerated(const std::shared_ptr<bdss::core::Student>& student, bool preferenceHit) {
    ++generatedCount_;
    if (student && student->isTakeaway()) {
        ++takeawayCount_;
    }
    if (preferenceHit) {
        ++preferenceHitCount_;
    }
}

void StatisticsLogger::logStudentFinished(const std::shared_ptr<bdss::core::Student>& student) {
    if (!student) {
        return;
    }
    ++finishedCount_;
    totalQueueWait_ += student->getQueueWaitingTime();
    totalSeatWait_ += student->getSeatWaitingTime();
    totalServiceTime_ += student->getServiceTime();
    totalDiningTime_ += student->isTakeaway() ? 0 : student->getDiningTime();
    totalSystemTime_ += student->getTotalSystemTime();
}

void StatisticsLogger::logStudentDropped(const std::shared_ptr<bdss::core::Student>&) {
    ++droppedCount_;
}

void StatisticsLogger::recordTick(const TickRecord& record) {
    tickRecords_.push_back(record);
}

void StatisticsLogger::reset() {
    generatedCount_ = 0;
    finishedCount_ = 0;
    droppedCount_ = 0;
    takeawayCount_ = 0;
    preferenceHitCount_ = 0;
    totalQueueWait_ = 0;
    totalSeatWait_ = 0;
    totalServiceTime_ = 0;
    totalDiningTime_ = 0;
    totalSystemTime_ = 0;
    tickRecords_.clear();
}

SummaryStats StatisticsLogger::getSummary() const {
    SummaryStats summary;
    summary.generatedStudents = generatedCount_;
    summary.finishedStudents = finishedCount_;
    summary.droppedStudents = droppedCount_;
    summary.takeawayStudents = takeawayCount_;

    if (!tickRecords_.empty()) {
        summary.maxQueueLength = std::max_element(tickRecords_.begin(), tickRecords_.end(), [](const auto& a, const auto& b) {
            return a.totalQueueLength < b.totalQueueLength;
        })->totalQueueLength;
        summary.maxWaitingForSeat = std::max_element(tickRecords_.begin(), tickRecords_.end(), [](const auto& a, const auto& b) {
            return a.waitingForSeatCount < b.waitingForSeatCount;
        })->waitingForSeatCount;
        const double totalUtil = std::accumulate(tickRecords_.begin(), tickRecords_.end(), 0.0, [](double total, const auto& record) {
            return total + record.seatUtilization;
        });
        summary.averageSeatUtilization = totalUtil / static_cast<double>(tickRecords_.size());
    }

    summary.averageQueueWait = safeAverage(totalQueueWait_, finishedCount_);
    summary.averageSeatWait = safeAverage(totalSeatWait_, finishedCount_);
    summary.averageServiceTime = safeAverage(totalServiceTime_, finishedCount_);
    summary.averageDiningTime = safeAverage(totalDiningTime_, finishedCount_);
    summary.averageTotalSystemTime = safeAverage(totalSystemTime_, finishedCount_);
    summary.preferenceHitRate = generatedCount_ <= 0 ? 0.0 : static_cast<double>(preferenceHitCount_) / static_cast<double>(generatedCount_);
    return summary;
}

void StatisticsLogger::exportCsv(const std::filesystem::path& filePath) const {
    std::ofstream output(filePath);
    if (!output) {
        throw std::runtime_error("Cannot write CSV file: " + filePath.string());
    }
    output << "time,total_queue_length,waiting_for_seat_count,occupied_seats,cleaning_seats,total_seats,"
              "finished_students,dropped_students,takeaway_students,in_system_students,seat_utilization,"
              "window_queue_lengths,window_served_counts\n";
    output << std::fixed << std::setprecision(4);
    for (const auto& record : tickRecords_) {
        output << record.time << ','
               << record.totalQueueLength << ','
               << record.waitingForSeatCount << ','
               << record.occupiedSeats << ','
               << record.cleaningSeats << ','
               << record.totalSeats << ','
               << record.finishedStudents << ','
               << record.droppedStudents << ','
               << record.takeawayStudents << ','
               << record.inSystemStudents << ','
               << record.seatUtilization << ','
               << '"' << joinInts(record.windowQueueLengths) << "\","
               << '"' << joinInts(record.windowServedCounts) << "\"\n";
    }
}

std::string StatisticsLogger::summaryText() const {
    const SummaryStats summary = getSummary();
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "generated_students=" << summary.generatedStudents << '\n'
        << "finished_students=" << summary.finishedStudents << '\n'
        << "dropped_students=" << summary.droppedStudents << '\n'
        << "takeaway_students=" << summary.takeawayStudents << '\n'
        << "max_queue_length=" << summary.maxQueueLength << '\n'
        << "max_waiting_for_seat=" << summary.maxWaitingForSeat << '\n'
        << "average_queue_wait_seconds=" << summary.averageQueueWait << '\n'
        << "average_seat_wait_seconds=" << summary.averageSeatWait << '\n'
        << "average_service_time_seconds=" << summary.averageServiceTime << '\n'
        << "average_dining_time_seconds=" << summary.averageDiningTime << '\n'
        << "average_total_system_time_seconds=" << summary.averageTotalSystemTime << '\n'
        << "average_seat_utilization_percent=" << summary.averageSeatUtilization * 100.0 << '\n'
        << "preference_hit_rate_percent=" << summary.preferenceHitRate * 100.0 << '\n';
    return oss.str();
}

} // namespace bdss::utils
