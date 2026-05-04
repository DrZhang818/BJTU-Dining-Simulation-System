#include "core/Student.h"

#include <sstream>

namespace bdss::core {

Student::Student(int id, int arrivalTime, int targetServiceTime, int targetDiningTime)
    : id_(id),
      state_(StudentState::Arrived),
      targetServiceTime_(targetServiceTime),
      targetDiningTime_(targetDiningTime),
      remainingServiceTime_(targetServiceTime),
      remainingDiningTime_(targetDiningTime),
      arrivalTime_(arrivalTime) {}

void Student::startQueuing(int time) {
    state_ = StudentState::Queuing;
    queueStartTime_ = time;
}

void Student::startServing(int time) {
    state_ = StudentState::Serving;
    serviceStartTime_ = time;
}

void Student::finishServiceAndWaitForSeat(int time) {
    state_ = StudentState::WaitingForSeat;
    serviceEndTime_ = time;
}

void Student::startDining(int time) {
    state_ = StudentState::Dining;
    diningStartTime_ = time;
}

void Student::finishDiningAndLeave(int time) {
    state_ = StudentState::Left;
    leaveTime_ = time;
}

void Student::decrementServiceTime() {
    if (remainingServiceTime_ > 0) {
        --remainingServiceTime_;
    }
}

void Student::decrementDiningTime() {
    if (remainingDiningTime_ > 0) {
        --remainingDiningTime_;
    }
}

int Student::getWaitTime() const {
    if (!serviceStartTime_.has_value()) {
        return -1;
    }
    return serviceStartTime_.value() - arrivalTime_;
}

int Student::getSeatWaitTime() const {
    if (!serviceEndTime_.has_value() || !diningStartTime_.has_value()) {
        return -1;
    }
    return diningStartTime_.value() - serviceEndTime_.value();
}

int Student::getServiceTime() const {
    if (!serviceStartTime_.has_value() || !serviceEndTime_.has_value()) {
        return -1;
    }
    return serviceEndTime_.value() - serviceStartTime_.value();
}

int Student::getTotalTime() const {
    if (!leaveTime_.has_value()) {
        return -1;
    }
    return leaveTime_.value() - arrivalTime_;
}

std::string Student::toString() const {
    std::ostringstream oss;
    oss << "Student[ID=" << id_
        << ", State=" << stateToString(state_)
        << ", Arrival=" << arrivalTime_
        << ", ServiceRemain=" << remainingServiceTime_
        << ", DiningRemain=" << remainingDiningTime_
        << "]";
    return oss.str();
}

std::string Student::stateToString(StudentState state) {
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
    default:
        return "Unknown";
    }
}

} // namespace bdss::core