#include "core/Canteen.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace bdss::core {

Canteen::Canteen(int rows, int cols)
    : rows_(std::max(1, rows)), cols_(std::max(1, cols)), seats_(static_cast<std::size_t>(rows_ * cols_)) {}

void Canteen::resize(int rows, int cols) {
    const int newRows = std::max(1, rows);
    const int newCols = std::max(1, cols);
    if (newRows == rows_ && newCols == cols_) {
        return;
    }

    std::vector<Seat> resized(static_cast<std::size_t>(newRows * newCols));
    const int copyRows = std::min(rows_, newRows);
    const int copyCols = std::min(cols_, newCols);
    for (int r = 0; r < copyRows; ++r) {
        for (int c = 0; c < copyCols; ++c) {
            resized[static_cast<std::size_t>(r * newCols + c)] = seatAt(r, c);
        }
    }
    rows_ = newRows;
    cols_ = newCols;
    seats_ = std::move(resized);
}

int Canteen::getOccupiedSeats() const {
    return static_cast<int>(std::count_if(seats_.begin(), seats_.end(), [](const Seat& seat) {
        return seat.state == SeatState::Occupied;
    }));
}

int Canteen::getCleaningSeats() const {
    return static_cast<int>(std::count_if(seats_.begin(), seats_.end(), [](const Seat& seat) {
        return seat.state == SeatState::Cleaning;
    }));
}

double Canteen::getSeatUtilization() const {
    const int total = getTotalSeats();
    return total == 0 ? 0.0 : static_cast<double>(getOccupiedSeats()) / static_cast<double>(total);
}

bool Canteen::trySeat(const std::shared_ptr<Student>& student, int currentTime, const Config& config) {
    if (!student || student->isTakeaway()) {
        return true;
    }

    int bestRow = -1;
    int bestCol = -1;
    double bestScore = -std::numeric_limits<double>::infinity();

    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            const Seat& seat = seatAt(row, col);
            if (seat.state != SeatState::Available) {
                continue;
            }
            const double score = scoreSeat(row, col, *student, config);
            if (score > bestScore) {
                bestScore = score;
                bestRow = row;
                bestCol = col;
            }
        }
    }

    if (bestRow < 0 || bestCol < 0) {
        return false;
    }

    Seat& seat = seatAt(bestRow, bestCol);
    seat.state = SeatState::Occupied;
    seat.occupant = student;
    seat.cleaningEndTime = -1;
    student->startDining(currentTime);
    return true;
}

std::vector<std::shared_ptr<Student>> Canteen::tick(int currentTime, const Config& config) {
    std::vector<std::shared_ptr<Student>> finished;
    for (Seat& seat : seats_) {
        if (seat.state == SeatState::Occupied && seat.occupant) {
            const int diningStart = seat.occupant->getDiningStartTime();
            if (diningStart >= 0 && currentTime - diningStart >= seat.occupant->getDiningTime()) {
                seat.occupant->finishDining(currentTime);
                finished.push_back(seat.occupant);
                seat.occupant.reset();
                if (config.enableCleaning && config.cleaningTime > 0) {
                    seat.state = SeatState::Cleaning;
                    seat.cleaningEndTime = currentTime + config.cleaningTime;
                } else {
                    seat.state = SeatState::Available;
                    seat.cleaningEndTime = -1;
                }
            }
        }
        if (seat.state == SeatState::Cleaning && currentTime >= seat.cleaningEndTime) {
            seat.state = SeatState::Available;
            seat.cleaningEndTime = -1;
        }
    }
    return finished;
}

std::vector<SeatSnapshot> Canteen::getSeatSnapshots() const {
    std::vector<SeatSnapshot> snapshots;
    snapshots.reserve(seats_.size());
    for (int row = 0; row < rows_; ++row) {
        for (int col = 0; col < cols_; ++col) {
            const Seat& seat = seatAt(row, col);
            SeatSnapshot snapshot;
            snapshot.row = row;
            snapshot.col = col;
            snapshot.state = seat.state;
            if (seat.occupant) {
                snapshot.studentId = seat.occupant->getId();
                snapshot.groupId = seat.occupant->getGroupId();
                snapshot.takeaway = seat.occupant->isTakeaway();
                snapshot.typeName = seat.occupant->getTypeName();
            }
            snapshots.push_back(std::move(snapshot));
        }
    }
    return snapshots;
}

double Canteen::scoreSeat(int row, int col, const Student& student, const Config& config) const {
    if (!config.enableSeatPreference) {
        return -static_cast<double>(row * cols_ + col);
    }

    double score = 0.0;
    const int windowCount = std::max(1, config.windowCount);
    const double normalizedWindow = static_cast<double>(std::max(0, student.getAssignedWindowId() - 1)) /
                                    static_cast<double>(windowCount);
    const double preferredCol = normalizedWindow * static_cast<double>(std::max(1, cols_ - 1));
    const double distance = std::abs(static_cast<double>(col) - preferredCol) + 0.35 * static_cast<double>(row);
    score -= config.nearWindowWeight * distance;

    if (config.enableGroupDining && student.getGroupId() > 0 && student.getGroupSize() > 1) {
        score += config.groupAdjacencyBonus * static_cast<double>(neighborSameGroupCount(row, col, student.getGroupId()));
    }
    score -= config.strangerSpacingPenalty * static_cast<double>(neighborStrangerCount(row, col, student.getGroupId()));
    return score;
}

int Canteen::neighborSameGroupCount(int row, int col, int groupId) const {
    if (groupId <= 0) {
        return 0;
    }
    int count = 0;
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) {
                continue;
            }
            const int nr = row + dr;
            const int nc = col + dc;
            if (nr < 0 || nr >= rows_ || nc < 0 || nc >= cols_) {
                continue;
            }
            const Seat& neighbor = seatAt(nr, nc);
            if (neighbor.occupant && neighbor.occupant->getGroupId() == groupId) {
                ++count;
            }
        }
    }
    return count;
}

int Canteen::neighborStrangerCount(int row, int col, int groupId) const {
    int count = 0;
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) {
                continue;
            }
            const int nr = row + dr;
            const int nc = col + dc;
            if (nr < 0 || nr >= rows_ || nc < 0 || nc >= cols_) {
                continue;
            }
            const Seat& neighbor = seatAt(nr, nc);
            if (neighbor.occupant && (groupId <= 0 || neighbor.occupant->getGroupId() != groupId)) {
                ++count;
            }
        }
    }
    return count;
}

std::string toString(SeatState state) {
    switch (state) {
        case SeatState::Available: return "available";
        case SeatState::Occupied: return "occupied";
        case SeatState::Cleaning: return "cleaning";
    }
    return "unknown";
}

} // namespace bdss::core
