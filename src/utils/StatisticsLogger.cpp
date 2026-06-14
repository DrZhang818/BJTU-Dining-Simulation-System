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
    std::ostringstream out;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out << ';';
        }
        out << values[i];
    }
    return out.str();
}

std::string percent(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value * 100.0 << '%';
    return out.str();
}

} // namespace

void StatisticsLogger::reset() {
    generatedStudents_ = 0;
    totalQueueWait_ = 0.0;
    queueWaitSamples_ = 0;
    totalSeatWait_ = 0.0;
    seatWaitSamples_ = 0;
    records_.clear();
}

void StatisticsLogger::setGeneratedStudents(int value) {
    generatedStudents_ = value;
}

void StatisticsLogger::addQueueWait(double seconds) {
    totalQueueWait_ += seconds;
    ++queueWaitSamples_;
}

void StatisticsLogger::addSeatWait(double seconds) {
    totalSeatWait_ += seconds;
    ++seatWaitSamples_;
}

void StatisticsLogger::recordTick(const TickRecord& record) {
    records_.push_back(record);
}

const std::vector<TickRecord>& StatisticsLogger::records() const {
    return records_;
}

Summary StatisticsLogger::summary() const {
    Summary s;
    s.generatedStudents = generatedStudents_;
    s.averageQueueWait = queueWaitSamples_ > 0 ? totalQueueWait_ / queueWaitSamples_ : 0.0;
    s.averageSeatWait = seatWaitSamples_ > 0 ? totalSeatWait_ / seatWaitSamples_ : 0.0;

    if (!records_.empty()) {
        const auto& last = records_.back();
        s.finishedStudents = last.finishedStudents;
        s.droppedStudents = last.droppedStudents;
        s.takeawayStudents = last.takeawayStudents;

        double seatUtilizationTotal = 0.0;
        for (const auto& r : records_) {
            seatUtilizationTotal += r.seatUtilization;
            if (r.totalQueueLength > s.maxQueueLength) {
                s.maxQueueLength = r.totalQueueLength;
                s.peakQueueTime = r.time;
            }
            if (r.waitingForSeatCount > s.maxWaitingForSeat) {
                s.maxWaitingForSeat = r.waitingForSeatCount;
                s.peakSeatWaitTime = r.time;
            }
        }
        s.averageSeatUtilization = seatUtilizationTotal / static_cast<double>(records_.size());
    }

    if (s.generatedStudents > 0) {
        s.finishRate = static_cast<double>(s.finishedStudents) / s.generatedStudents;
        s.dropRate = static_cast<double>(s.droppedStudents) / s.generatedStudents;
    }

    return s;
}

std::string StatisticsLogger::summaryText() const {
    const auto s = summary();
    std::ostringstream out;
    out << "Summary\n"
        << "  Generated students: " << s.generatedStudents << '\n'
        << "  Finished students: " << s.finishedStudents << '\n'
        << "  Dropped students: " << s.droppedStudents << '\n'
        << "  Takeaway students: " << s.takeawayStudents << '\n'
        << "  Finish rate: " << percent(s.finishRate) << '\n'
        << "  Drop rate: " << percent(s.dropRate) << '\n'
        << "  Max queue length: " << s.maxQueueLength << " at " << s.peakQueueTime << "s\n"
        << "  Max waiting for seat: " << s.maxWaitingForSeat << " at " << s.peakSeatWaitTime << "s\n"
        << "  Average queue wait: " << std::fixed << std::setprecision(1) << s.averageQueueWait << "s\n"
        << "  Average seat wait: " << std::fixed << std::setprecision(1) << s.averageSeatWait << "s\n"
        << "  Average seat utilization: " << percent(s.averageSeatUtilization) << '\n';
    return out.str();
}

void StatisticsLogger::exportCsv(const std::filesystem::path& path) const {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Cannot write CSV file: " + path.string());
    }

    output << "time,total_queue_length,waiting_for_seat_count,occupied_seats,cleaning_seats,total_seats,"
              "finished_students,dropped_students,takeaway_students,in_system_students,seat_utilization,"
              "window_queue_lengths,window_served_counts\n";

    output << std::fixed << std::setprecision(4);
    for (const auto& r : records_) {
        output << r.time << ','
               << r.totalQueueLength << ','
               << r.waitingForSeatCount << ','
               << r.occupiedSeats << ','
               << r.cleaningSeats << ','
               << r.totalSeats << ','
               << r.finishedStudents << ','
               << r.droppedStudents << ','
               << r.takeawayStudents << ','
               << r.inSystemStudents << ','
               << r.seatUtilization << ','
               << '"' << joinInts(r.windowQueueLengths) << '"' << ','
               << '"' << joinInts(r.windowServedCounts) << '"' << '\n';
    }
}

} // namespace bdss::utils
