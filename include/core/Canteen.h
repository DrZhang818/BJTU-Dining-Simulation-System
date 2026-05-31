#pragma once

#include "core/Student.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace bdss::core {

class Canteen {
public:
    Canteen(int rows, int cols);

    int rows() const noexcept;
    int cols() const noexcept;
    int capacity() const noexcept;
    int getOccupiedSeats() const noexcept;
    double utilization() const noexcept;
    bool hasEmptySeat() const noexcept;

    bool trySeat(const std::shared_ptr<Student>& student, int now);
    std::vector<std::shared_ptr<Student>> tick(int now);
    std::string renderSeatMatrix() const;

private:
    std::pair<int, int> findEmptySeat() const;

    int rows_;
    int cols_;
    int occupiedSeats_ = 0;
    std::vector<std::vector<std::shared_ptr<Student>>> seats_;
};

} // namespace bdss::core
