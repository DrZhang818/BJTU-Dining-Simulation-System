#include "core/Canteen.h"

namespace bdss::core {

Canteen::Canteen(int rows, int cols)
    : rows_(rows), cols_(cols), seats_(rows, std::vector<std::shared_ptr<Student>>(cols, nullptr)) {}

bool Canteen::trySeat(const std::shared_ptr<Student>& student, int currentTime) {
    if (!student) {
        return false;
    }

    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            if (seats_[r][c] == nullptr) {
                student->startDining(currentTime);
                seats_[r][c] = student;
                return true;
            }
        }
    }

    return false;
}

std::vector<std::shared_ptr<Student>> Canteen::tick(int currentTime) {
    std::vector<std::shared_ptr<Student>> finishedStudents;

    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            auto student = seats_[r][c];
            if (!student) {
                continue;
            }

            student->decrementDiningTime();

            if (student->getRemainingDiningTime() <= 0) {
                student->finishDiningAndLeave(currentTime + 1);
                finishedStudents.push_back(student);
                seats_[r][c] = nullptr;
            }
        }
    }

    return finishedStudents;
}

int Canteen::getOccupiedSeats() const {
    int occupied = 0;

    for (const auto& row : seats_) {
        for (const auto& seat : row) {
            if (seat != nullptr) {
                ++occupied;
            }
        }
    }

    return occupied;
}

double Canteen::getSeatUtilization() const {
    if (getTotalSeats() <= 0) {
        return 0.0;
    }

    return static_cast<double>(getOccupiedSeats()) / static_cast<double>(getTotalSeats());
}

std::vector<std::vector<int>> Canteen::getSeatSnapshot() const {
    std::vector<std::vector<int>> snapshot(rows_, std::vector<int>(cols_, 0));

    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            if (seats_[r][c] != nullptr) {
                snapshot[r][c] = seats_[r][c]->getId();
            }
        }
    }

    return snapshot;
}

} // namespace bdss::core