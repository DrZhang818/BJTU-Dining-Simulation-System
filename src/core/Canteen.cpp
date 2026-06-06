#include "core/Canteen.h"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace bdss::core {

Canteen::Canteen(int rows, int cols, int cleaningTime,
                  double windowProxWeight, double groupProxWeight,
                  double isolationWeight)
    : rows_(rows),
      cols_(cols),
      cleaningTime_(cleaningTime),
      windowProxWeight_(windowProxWeight),
      groupProxWeight_(groupProxWeight),
      isolationWeight_(isolationWeight),
      seats_(static_cast<std::size_t>(rows),
             std::vector<Seat>(static_cast<std::size_t>(cols))) {
    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("Canteen rows and cols must be positive");
    }
}

double Canteen::utilization() const noexcept {
    if (capacity() == 0) return 0.0;
    return static_cast<double>(occupiedSeats_) / static_cast<double>(capacity()) * 100.0;
}

bool Canteen::hasEmptySeat() const noexcept {
    for (const auto& row : seats_) {
        for (const auto& seat : row) {
            if (seat.isEmpty()) return true;
        }
    }
    return false;
}

bool Canteen::isSeatOccupied(int row, int col) const noexcept {
    if (row < 0 || row >= rows_ || col < 0 || col >= cols_) return false;
    return seats_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)].student != nullptr;
}

int Canteen::countAdjacentStrangers(int r, int c, int studentGroupId) const noexcept {
    int count = 0;
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            const int nr = r + dr, nc = c + dc;
            if (nr < 0 || nr >= rows_ || nc < 0 || nc >= cols_) continue;
            const auto& seat = seats_[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)];
            if (seat.student && seat.student->getGroupId() != studentGroupId) {
                ++count;
            }
        }
    }
    return count;
}

std::pair<int, int> Canteen::findBestSeat(int windowVirtualCol,
                                           int groupCentroidRow,
                                           int groupCentroidCol,
                                           int studentGroupId) const {
    int bestR = -1, bestC = -1;
    double bestScore = std::numeric_limits<double>::max();

    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            const auto& seat = seats_[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
            if (!seat.isEmpty()) continue;

            double score = 0.0;

            // 窗口就近：曼哈顿距离（窗口虚拟在 row=-1）
            if (windowVirtualCol >= 0) {
                score += windowProxWeight_ * static_cast<double>(r + 1 + std::abs(c - windowVirtualCol));
            }

            // 结伴邻桌：到已就座组员质心的曼哈顿距离
            if (groupCentroidRow >= 0) {
                score += groupProxWeight_ * static_cast<double>(std::abs(r - groupCentroidRow) + std::abs(c - groupCentroidCol));
            }

            // 陌生人距离感
            score += isolationWeight_ * static_cast<double>(countAdjacentStrangers(r, c, studentGroupId));

            if (score < bestScore) {
                bestScore = score;
                bestR = r;
                bestC = c;
            }
        }
    }
    return {bestR, bestC};
}

bool Canteen::trySeat(const std::shared_ptr<Student>& student, int now,
                       int windowVirtualCol) {
    if (!student) {
        throw std::invalid_argument("cannot seat null student");
    }
    const int gid = student->getGroupId();
    auto [row, col] = findBestSeat(windowVirtualCol, -1, -1, gid);
    if (row < 0) {
        return false;
    }
    auto& seat = seats_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
    seat.student = student;
    seat.cleaningLeft = 0;
    ++occupiedSeats_;
    student->startDining(now, row, col);
    return true;
}

int Canteen::tryGroupSeat(const std::vector<std::shared_ptr<Student>>& group, int now,
                           int windowVirtualCol) {
    const int need = static_cast<int>(group.size());
    if (need <= 0) return 0;
    if (need == 1) {
        return trySeat(group[0], now, windowVirtualCol) ? 1 : 0;
    }

    // 先尝试相邻列（同行连续空位）
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c <= cols_ - need; ++c) {
            bool canPlace = true;
            for (int dc = 0; dc < need; ++dc) {
                if (!seats_[static_cast<std::size_t>(r)][static_cast<std::size_t>(c + dc)].isEmpty()) {
                    canPlace = false;
                    break;
                }
            }
            if (canPlace) {
                for (int dc = 0; dc < need; ++dc) {
                    auto& seat = seats_[static_cast<std::size_t>(r)][static_cast<std::size_t>(c + dc)];
                    seat.student = group[static_cast<std::size_t>(dc)];
                    seat.cleaningLeft = 0;
                    ++occupiedSeats_;
                    group[static_cast<std::size_t>(dc)]->startDining(now, r, c + dc);
                }
                return need;
            }
        }
    }

    // 降级：个人就座，使用评分算法
    int seated = 0;
    // 先计算已就座组员的质心（如果有的话）
    int grSum = 0, gcSum = 0, gCount = 0;
    for (const auto& s : group) {
        if (s->getState() == StudentState::Dining) {
            grSum += s->getSeatRow();
            gcSum += s->getSeatCol();
            ++gCount;
        }
    }
    const int centroidR = gCount > 0 ? grSum / gCount : -1;
    const int centroidC = gCount > 0 ? gcSum / gCount : -1;
    const int gid = group[0]->getGroupId();

    for (const auto& student : group) {
        if (student->getState() == StudentState::Dining) continue;
        auto [row, col] = findBestSeat(windowVirtualCol, centroidR, centroidC, gid);
        if (row < 0) break;
        auto& seat = seats_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
        seat.student = student;
        seat.cleaningLeft = 0;
        ++occupiedSeats_;
        student->startDining(now, row, col);
        ++seated;
    }
    return seated;
}

std::vector<std::shared_ptr<Student>> Canteen::tick(int now) {
    std::vector<std::shared_ptr<Student>> finished;

    // 推进清洁计时
    for (auto& row : seats_) {
        for (auto& seat : row) {
            if (seat.cleaningLeft > 0) --seat.cleaningLeft;
        }
    }

    // 推进就餐
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            auto& seat = seats_[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
            if (!seat.student) continue;
            if (seat.student->advanceDiningOneSecond()) {
                seat.student->finishDiningAndLeave(now + 1);
                finished.push_back(seat.student);
                seat.student.reset();
                --occupiedSeats_;
                if (cleaningTime_ > 0) {
                    seat.cleaningLeft = cleaningTime_;
                }
            }
        }
    }
    return finished;
}

std::string Canteen::renderSeatMatrix() const {
    std::ostringstream os;
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            const auto& seat = seats_[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
            if (seat.student) {
                os << 'X';
            } else if (seat.cleaningLeft > 0) {
                os << '~';
            } else {
                os << '.';
            }
        }
        if (r + 1 < rows_) os << '\n';
    }
    return os.str();
}

} // namespace bdss::core
