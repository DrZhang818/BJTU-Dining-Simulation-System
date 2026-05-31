#include "utils/StatisticsLogger.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace bdss::utils {

void StatisticsLogger::clear() {
    snapshots_.clear();
    summary_ = SimulationSummary{};
}

void StatisticsLogger::record(int time,
                              int totalQueueLength,
                              int waitingForSeatCount,
                              int occupiedSeats,
                              int finishedStudents,
                              double seatUtilization) {
    snapshots_.push_back(Snapshot{time,
                                  totalQueueLength,
                                  waitingForSeatCount,
                                  occupiedSeats,
                                  finishedStudents,
                                  seatUtilization});
    summary_.maxQueueLength = std::max(summary_.maxQueueLength, totalQueueLength);
    summary_.maxWaitingForSeatCount = std::max(summary_.maxWaitingForSeatCount, waitingForSeatCount);
}

void StatisticsLogger::finalize(const std::vector<std::shared_ptr<bdss::core::Student>>& finishedStudents) {
    summary_.finishedStudents = static_cast<int>(finishedStudents.size());

    if (!finishedStudents.empty()) {
        auto average = [&](auto getter) {
            double sum = 0.0;
            for (const auto& student : finishedStudents) {
                sum += static_cast<double>(getter(*student));
            }
            return sum / static_cast<double>(finishedStudents.size());
        };
        summary_.averageQueueWaitTime = average([](const auto& s) { return s.getQueueWaitTime(); });
        summary_.averageSeatWaitTime = average([](const auto& s) { return s.getSeatWaitTime(); });
        summary_.averageServiceTime = average([](const auto& s) { return s.getActualServiceTime(); });
        summary_.averageTotalTimeInCanteen = average([](const auto& s) { return s.getTotalTime(); });
    }

    if (!snapshots_.empty()) {
        const auto sum = std::accumulate(snapshots_.begin(), snapshots_.end(), 0.0, [](double total, const Snapshot& snapshot) {
            return total + snapshot.seatUtilization;
        });
        summary_.averageSeatUtilization = sum / static_cast<double>(snapshots_.size());
    }
}

const SimulationSummary& StatisticsLogger::summary() const noexcept {
    return summary_;
}

const std::vector<Snapshot>& StatisticsLogger::snapshots() const noexcept {
    return snapshots_;
}

int StatisticsLogger::getFinishedCount() const noexcept {
    return summary_.finishedStudents;
}

void StatisticsLogger::exportCSV(const std::filesystem::path& outputPath) const {
    std::ofstream output(outputPath);
    if (!output) {
        throw std::runtime_error("cannot write CSV file: " + outputPath.string());
    }
    output << "time,total_queue_length,waiting_for_seat_count,occupied_seats,finished_students,seat_utilization\n";
    output << std::fixed << std::setprecision(4);
    for (const auto& snapshot : snapshots_) {
        output << snapshot.time << ','
               << snapshot.totalQueueLength << ','
               << snapshot.waitingForSeatCount << ','
               << snapshot.occupiedSeats << ','
               << snapshot.finishedStudents << ','
               << snapshot.seatUtilization << '\n';
    }
}

void StatisticsLogger::printSummary(std::ostream& os) const {
    os << summaryText();
}

std::string StatisticsLogger::summaryText() const {
    std::ostringstream os;
    os << std::fixed << std::setprecision(4);
    os << "========== BDSS Simulation Summary ==========\n"
       << "Finished students: " << summary_.finishedStudents << '\n'
       << "Max queue length: " << summary_.maxQueueLength << '\n'
       << "Max waiting-for-seat count: " << summary_.maxWaitingForSeatCount << '\n'
       << "Average queue wait time: " << summary_.averageQueueWaitTime << " seconds\n"
       << "Average seat wait time: " << summary_.averageSeatWaitTime << " seconds\n"
       << "Average service time: " << summary_.averageServiceTime << " seconds\n"
       << "Average total time in canteen: " << summary_.averageTotalTimeInCanteen << " seconds\n"
       << "Average seat utilization: " << summary_.averageSeatUtilization << "%\n"
       << "============================================\n";
    return os.str();
}

} // namespace bdss::utils
