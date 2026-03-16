#include "core/Student.h"

namespace bdss::core {

Student::Student(int id, int arrivalTime, int targetServiceTime, int targetDiningTime)
    : id_(id),
      state_(StudentState::Arrived),
      remainingServiceTime_(targetServiceTime),
      remainingDiningTime_(targetDiningTime),
      arrivalTime_(arrivalTime),
      startDiningTime_(std::nullopt),
      leaveTime_(std::nullopt) {}

void Student::startQueuing() { 
    state_ = StudentState::Queuing; 
}

void Student::finishQueuingAndStartDining(int time) {
    state_ = StudentState::Dining;
    startDiningTime_ = time;
}

void Student::finishDiningAndLeave(int time) {
    state_ = StudentState::Left;
    leaveTime_ = time;
}

int Student::getWaitTime() const {
    if (!startDiningTime_.has_value()) { return -1; }
    return startDiningTime_.value() - arrivalTime_;
}

int Student::getTotalTime() const {
    if (!leaveTime_.has_value()) { return -1; }
    return leaveTime_.value() - arrivalTime_;
}

std::string Student::toString() const {
    std::string stateStr;
    switch (state_) {
        case StudentState::Arrived: stateStr = "Arrived"; break;
        case StudentState::Queuing: stateStr = "Queuing"; break;
        case StudentState::Dining:  stateStr = "Dining";  break;
        case StudentState::Left:    stateStr = "Left";    break;
    }
    return std::format("Student[ID:{}, State:{}, Arrival:{}]", id_, stateStr, arrivalTime_);
}

}  // namespace bdss::core