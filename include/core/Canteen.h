#pragma once

#include "core/Student.h"

#include <memory>
#include <vector>

namespace bdss::core {

class Canteen {
public:
    Canteen(int rows, int cols);

    bool trySeat(const std::shared_ptr<Student>& student, int currentTime);
    std::vector<std::shared_ptr<Student>> tick(int currentTime);

    int getRows() const { return rows_; }
    int getCols() const { return cols_; }
    int getTotalSeats() const { return rows_ * cols_; }
    int getOccupiedSeats() const;
    double getSeatUtilization() const;

    std::vector<std::vector<int>> getSeatSnapshot() const;

private:
    int rows_;
    int cols_;
    std::vector<std::vector<std::shared_ptr<Student>>> seats_;
};

} // namespace bdss::core