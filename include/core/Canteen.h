#pragma once

#include "core/Student.h"
#include "core/Config.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace bdss::core {

struct Seat {
    std::shared_ptr<Student> student;
    int cleaningLeft = 0;
    
    bool isEmpty() const noexcept { return !student && cleaningLeft <= 0; }
};

class Canteen {
public:
    Canteen(int rows, int cols, int cleaningTime = 0,
            double windowProxWeight = 0.0,
            double groupProxWeight  = 0.0,
            double isolationWeight  = 0.0);

    int rows() const noexcept { return rows_; }
    int cols() const noexcept { return cols_; }
    int capacity() const noexcept { return rows_ * cols_; }
    int getOccupiedSeats() const noexcept { return occupiedSeats_; }
    double utilization() const noexcept;
    bool hasEmptySeat() const noexcept;

    bool isSeatOccupied(int row, int col) const noexcept;
    bool trySeat(const std::shared_ptr<Student>& student, int now,
                 int windowVirtualCol = -1);
    int tryGroupSeat(const std::vector<std::shared_ptr<Student>>& group, int now,
                     int windowVirtualCol = -1);
    std::vector<std::shared_ptr<Student>> tick(int now);
    std::string renderSeatMatrix() const;

private:
    // 三项评分选座，返回最优空位坐标（-1,-1 表示无空位）
    std::pair<int, int> findBestSeat(int windowVirtualCol,
                                     int groupCentroidRow,
                                     int groupCentroidCol,
                                     int studentGroupId) const;
    // 计算 (r,c) 周围 8 格中非同组占用的数量
    int countAdjacentStrangers(int r, int c, int studentGroupId) const noexcept;

    int rows_;
    int cols_;
    int cleaningTime_ = 0;
    int occupiedSeats_ = 0;
    double windowProxWeight_ = 0.0;
    double groupProxWeight_  = 0.0;
    double isolationWeight_  = 0.0;
    std::vector<std::vector<Seat>> seats_;
};

} // namespace bdss::core
