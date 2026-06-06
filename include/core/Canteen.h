#pragma once

#include "core/Config.h"
#include "core/Student.h"

#include <memory>
#include <vector>

namespace bdss::core {

enum class SeatState {
    Available,
    Occupied,
    Cleaning
};

struct SeatSnapshot {
    int row = 0;
    int col = 0;
    SeatState state = SeatState::Available;
    int studentId = -1;
    int groupId = 0;
    bool takeaway = false;
    std::string typeName;
};

class Canteen {
public:
    Canteen(int rows, int cols);
    void resize(int rows, int cols);

    int getRows() const { return rows_; }
    int getCols() const { return cols_; }
    int getTotalSeats() const { return rows_ * cols_; }
    int getOccupiedSeats() const;
    int getCleaningSeats() const;
    double getSeatUtilization() const;

    bool trySeat(const std::shared_ptr<Student>& student, int currentTime, const Config& config);
    std::vector<std::shared_ptr<Student>> tick(int currentTime, const Config& config);
    std::vector<SeatSnapshot> getSeatSnapshots() const;

private:
    struct Seat {
        SeatState state = SeatState::Available;
        std::shared_ptr<Student> occupant;
        int cleaningEndTime = -1;
    };

    double scoreSeat(int row, int col, const Student& student, const Config& config) const;
    int neighborSameGroupCount(int row, int col, int groupId) const;
    int neighborStrangerCount(int row, int col, int groupId) const;
    const Seat& seatAt(int row, int col) const { return seats_[static_cast<std::size_t>(row * cols_ + col)]; }
    Seat& seatAt(int row, int col) { return seats_[static_cast<std::size_t>(row * cols_ + col)]; }

    int rows_ = 0;
    int cols_ = 0;
    std::vector<Seat> seats_;
};

std::string toString(SeatState state);

} // namespace bdss::core
