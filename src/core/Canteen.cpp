#include "core/Canteen.h"

#include <sstream>
#include <stdexcept>

namespace bdss::core {

Canteen::Canteen(int rows, int cols)
    : rows_(rows), cols_(cols), seats_(static_cast<std::size_t>(rows), std::vector<std::shared_ptr<Student>>(static_cast<std::size_t>(cols))) {
    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("Canteen rows and cols must be positive");
    }
}

int Canteen::rows() const noexcept { return rows_; }
int Canteen::cols() const noexcept { return cols_; }
int Canteen::capacity() const noexcept { return rows_ * cols_; }
int Canteen::getOccupiedSeats() const noexcept { return occupiedSeats_; }

double Canteen::utilization() const noexcept {
    if (capacity() == 0) {
        return 0.0;
    }
    return static_cast<double>(occupiedSeats_) / static_cast<double>(capacity()) * 100.0;
}

bool Canteen::hasEmptySeat() const noexcept {
    return occupiedSeats_ < capacity();
}

std::pair<int, int> Canteen::findEmptySeat() const {
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            if (!seats_[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)]) {
                return {r, c};
            }
        }
    }
    return {-1, -1};
}

bool Canteen::trySeat(const std::shared_ptr<Student>& student, int now) {
    if (!student) {
        throw std::invalid_argument("cannot seat null student");
    }
    const auto [row, col] = findEmptySeat();
    if (row < 0) {
        return false;
    }
    seats_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = student;
    ++occupiedSeats_;
    student->startDining(now, row, col);
    return true;
}

std::vector<std::shared_ptr<Student>> Canteen::tick(int now) {
    std::vector<std::shared_ptr<Student>> finished;
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            auto& student = seats_[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
            if (!student) {
                continue;
            }
            if (student->advanceDiningOneSecond()) {
                student->finishDiningAndLeave(now + 1);
                finished.push_back(student);
                student.reset();
                --occupiedSeats_;
            }
        }
    }
    return finished;
}

std::string Canteen::renderSeatMatrix() const {
    std::ostringstream os;
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            os << (seats_[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] ? 'X' : '.');
        }
        if (r + 1 < rows_) {
            os << '\n';
        }
    }
    return os.str();
}

} // namespace bdss::core
