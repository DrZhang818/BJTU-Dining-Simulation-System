#include "core/Student.h"

#include <algorithm>
#include <utility>

namespace bdss::core {

Student::Student(int id,
                 int arrivalTime,
                 int serviceTime,
                 int diningTime,
                 std::string typeName,
                 std::string preferredCategory,
                 bool takeaway,
                 int patienceSeconds,
                 int groupId,
                 int groupSize)
    : id_(id),
      arrivalTime_(arrivalTime),
      serviceTime_(std::max(1, serviceTime)),
      diningTime_(std::max(1, diningTime)),
      typeName_(std::move(typeName)),
      preferredCategory_(std::move(preferredCategory)),
      takeaway_(takeaway),
      patienceSeconds_(std::max(1, patienceSeconds)),
      groupId_(groupId),
      groupSize_(std::max(1, groupSize)),
      queueEntryTime_(arrivalTime) {}

void Student::enterQueue(int currentTime, int windowId) {
    if (state_ == StudentState::Queued && queueEntryTime_ >= 0 && currentTime > queueEntryTime_) {
        accumulatedQueueWait_ += currentTime - queueEntryTime_;
    }
    queueEntryTime_ = currentTime;
    assignedWindowId_ = windowId;
    state_ = StudentState::Queued;
}

void Student::startService(int currentTime) {
    state_ = StudentState::BeingServed;
    if (serviceStartTime_ < 0) {
        if (queueEntryTime_ >= 0) {
            accumulatedQueueWait_ += std::max(0, currentTime - queueEntryTime_);
        }
        serviceStartTime_ = currentTime;
    }
}

void Student::finishService(int currentTime) {
    serviceEndTime_ = currentTime;
    if (takeaway_) {
        finishedTime_ = currentTime;
        state_ = StudentState::Finished;
    } else {
        state_ = StudentState::WaitingForSeat;
    }
}

void Student::startWaitingForSeat(int currentTime) {
    seatWaitStartTime_ = currentTime;
    state_ = StudentState::WaitingForSeat;
}

void Student::startDining(int currentTime) {
    diningStartTime_ = currentTime;
    state_ = StudentState::Dining;
}

void Student::finishDining(int currentTime) {
    finishedTime_ = currentTime;
    state_ = StudentState::Finished;
}

void Student::drop(int currentTime) {
    droppedTime_ = currentTime;
    state_ = StudentState::Dropped;
}

int Student::queueWaitingTimeAt(int currentTime) const {
    if (serviceStartTime_ >= 0) {
        return accumulatedQueueWait_;
    }
    return accumulatedQueueWait_ + currentQueueWaitingTimeAt(currentTime);
}

int Student::currentQueueWaitingTimeAt(int currentTime) const {
    return std::max(0, currentTime - queueEntryTime_);
}

bool Student::isPatienceExpired(int currentTime) const {
    return state_ == StudentState::Queued && currentQueueWaitingTimeAt(currentTime) >= patienceSeconds_;
}

int Student::getQueueWaitingTime() const {
    return accumulatedQueueWait_;
}

int Student::getSeatWaitingTime() const {
    if (takeaway_) {
        return 0;
    }
    if (seatWaitStartTime_ >= 0 && diningStartTime_ >= 0) {
        return diningStartTime_ - seatWaitStartTime_;
    }
    return 0;
}

int Student::getTotalSystemTime() const {
    if (finishedTime_ >= 0) {
        return finishedTime_ - arrivalTime_;
    }
    if (droppedTime_ >= 0) {
        return droppedTime_ - arrivalTime_;
    }
    return 0;
}

std::string toString(StudentState state) {
    switch (state) {
        case StudentState::Queued: return "queued";
        case StudentState::BeingServed: return "being_served";
        case StudentState::WaitingForSeat: return "waiting_for_seat";
        case StudentState::Dining: return "dining";
        case StudentState::Finished: return "finished";
        case StudentState::Dropped: return "dropped";
    }
    return "unknown";
}

} // namespace bdss::core
