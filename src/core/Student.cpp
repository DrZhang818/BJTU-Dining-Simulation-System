#include "core/Student.h"

#include <algorithm>
#include <ostream>
#include <stdexcept>

namespace bdss::core {

std::string toString(StudentState state) {
    switch (state) {
    case StudentState::Arrived:
        return "Arrived";
    case StudentState::Queuing:
        return "Queuing";
    case StudentState::Serving:
        return "Serving";
    case StudentState::WaitingForSeat:
        return "WaitingForSeat";
    case StudentState::Dining:
        return "Dining";
    case StudentState::Left:
        return "Left";
    }
    return "Unknown";
}

Student::Student(int id, int arrivalTime, int serviceTime, int diningTime)
    : id_(id),
      arrivalTime_(arrivalTime),
      serviceTime_(std::max(1, serviceTime)),
      diningTime_(std::max(1, diningTime)),
      remainingServiceTime_(std::max(1, serviceTime)),
      remainingDiningTime_(std::max(1, diningTime)) {
    if (id <= 0) {
        throw std::invalid_argument("Student id must be positive");
    }
    if (arrivalTime < 0) {
        throw std::invalid_argument("arrivalTime must be non-negative");
    }
}

int Student::getId() const noexcept { return id_; }
StudentState Student::getState() const noexcept { return state_; }
int Student::getArrivalTime() const noexcept { return arrivalTime_; }
int Student::getQueueStartTime() const noexcept { return queueStartTime_; }
int Student::getServiceStartTime() const noexcept { return serviceStartTime_; }
int Student::getServiceEndTime() const noexcept { return serviceEndTime_; }
int Student::getDiningStartTime() const noexcept { return diningStartTime_; }
int Student::getLeaveTime() const noexcept { return leaveTime_; }
int Student::getServiceTime() const noexcept { return serviceTime_; }
int Student::getDiningTime() const noexcept { return diningTime_; }
int Student::getRemainingServiceTime() const noexcept { return remainingServiceTime_; }
int Student::getRemainingDiningTime() const noexcept { return remainingDiningTime_; }
int Student::getSeatRow() const noexcept { return seatRow_; }
int Student::getSeatCol() const noexcept { return seatCol_; }

void Student::setServiceTime(int seconds) {
    if (state_ != StudentState::Arrived) {
        throw std::logic_error("service time can only be adjusted before queuing");
    }
    serviceTime_ = std::max(1, seconds);
    remainingServiceTime_ = serviceTime_;
}

void Student::startQueuing(int now) {
    if (state_ != StudentState::Arrived) {
        throw std::logic_error("student can queue only after arrival");
    }
    queueStartTime_ = now;
    state_ = StudentState::Queuing;
}

void Student::startServing(int now) {
    if (state_ != StudentState::Queuing) {
        throw std::logic_error("student can start service only from queue");
    }
    serviceStartTime_ = now;
    state_ = StudentState::Serving;
}

bool Student::advanceServiceOneSecond() {
    if (state_ != StudentState::Serving) {
        throw std::logic_error("cannot advance service for non-serving student");
    }
    if (remainingServiceTime_ > 0) {
        --remainingServiceTime_;
    }
    return remainingServiceTime_ <= 0;
}

void Student::finishServiceAndWaitForSeat(int now) {
    if (state_ != StudentState::Serving && state_ != StudentState::Queuing) {
        throw std::logic_error("student cannot finish service from current state");
    }
    remainingServiceTime_ = 0;
    if (serviceStartTime_ < 0) {
        serviceStartTime_ = now;
    }
    serviceEndTime_ = now;
    state_ = StudentState::WaitingForSeat;
}

void Student::startDining(int now, int row, int col) {
    if (state_ != StudentState::WaitingForSeat) {
        throw std::logic_error("student can start dining only after service");
    }
    diningStartTime_ = now;
    seatRow_ = row;
    seatCol_ = col;
    state_ = StudentState::Dining;
}

bool Student::advanceDiningOneSecond() {
    if (state_ != StudentState::Dining) {
        throw std::logic_error("cannot advance dining for non-dining student");
    }
    if (remainingDiningTime_ > 0) {
        --remainingDiningTime_;
    }
    return remainingDiningTime_ <= 0;
}

void Student::finishDiningAndLeave(int now) {
    if (state_ != StudentState::Dining) {
        throw std::logic_error("student can leave only after dining");
    }
    remainingDiningTime_ = 0;
    leaveTime_ = now;
    state_ = StudentState::Left;
}

int Student::getQueueWaitTime() const noexcept {
    if (serviceStartTime_ < 0) {
        return 0;
    }
    const auto start = queueStartTime_ >= 0 ? queueStartTime_ : arrivalTime_;
    return std::max(0, serviceStartTime_ - start);
}

int Student::getSeatWaitTime() const noexcept {
    if (serviceEndTime_ < 0 || diningStartTime_ < 0) {
        return 0;
    }
    return std::max(0, diningStartTime_ - serviceEndTime_);
}

int Student::getActualServiceTime() const noexcept {
    if (serviceStartTime_ < 0 || serviceEndTime_ < 0) {
        return 0;
    }
    return std::max(0, serviceEndTime_ - serviceStartTime_);
}

int Student::getActualDiningTime() const noexcept {
    if (diningStartTime_ < 0 || leaveTime_ < 0) {
        return 0;
    }
    return std::max(0, leaveTime_ - diningStartTime_);
}

int Student::getTotalTime() const noexcept {
    if (leaveTime_ < 0) {
        return 0;
    }
    return std::max(0, leaveTime_ - arrivalTime_);
}

std::ostream& operator<<(std::ostream& os, const Student& student) {
    os << "Student{id=" << student.getId()
       << ", state=" << toString(student.getState())
       << ", arrival=" << student.getArrivalTime()
       << ", service=" << student.getServiceTime()
       << ", dining=" << student.getDiningTime()
       << '}';
    return os;
}

} // namespace bdss::core
