#pragma once

#include <string>

namespace bdss::core {

enum class StudentState {
    Queued,
    BeingServed,
    WaitingForSeat,
    Dining,
    Finished,
    Dropped
};

class Student {
public:
    Student(int id,
            int arrivalTime,
            int serviceTime,
            int diningTime,
            std::string typeName,
            std::string preferredCategory,
            bool takeaway,
            int patienceSeconds,
            int groupId,
            int groupSize);

    int getId() const { return id_; }
    int getArrivalTime() const { return arrivalTime_; }
    int getServiceTime() const { return serviceTime_; }
    int getDiningTime() const { return diningTime_; }
    int getServiceStartTime() const { return serviceStartTime_; }
    int getServiceEndTime() const { return serviceEndTime_; }
    int getSeatWaitStartTime() const { return seatWaitStartTime_; }
    int getDiningStartTime() const { return diningStartTime_; }
    int getFinishedTime() const { return finishedTime_; }
    int getDroppedTime() const { return droppedTime_; }
    int getAssignedWindowId() const { return assignedWindowId_; }
    int getGroupId() const { return groupId_; }
    int getGroupSize() const { return groupSize_; }
    int getPatienceSeconds() const { return patienceSeconds_; }
    const std::string& getTypeName() const { return typeName_; }
    const std::string& getPreferredCategory() const { return preferredCategory_; }
    bool isTakeaway() const { return takeaway_; }
    StudentState getState() const { return state_; }

    void setAssignedWindowId(int windowId) { assignedWindowId_ = windowId; }
    void enterQueue(int currentTime, int windowId);
    void startService(int currentTime);
    void finishService(int currentTime);
    void startWaitingForSeat(int currentTime);
    void startDining(int currentTime);
    void finishDining(int currentTime);
    void drop(int currentTime);

    int queueWaitingTimeAt(int currentTime) const;
    int currentQueueWaitingTimeAt(int currentTime) const;
    bool isPatienceExpired(int currentTime) const;
    int getQueueWaitingTime() const;
    int getSeatWaitingTime() const;
    int getTotalSystemTime() const;

private:
    int id_ = 0;
    int arrivalTime_ = 0;
    int serviceTime_ = 1;
    int diningTime_ = 1;
    std::string typeName_;
    std::string preferredCategory_;
    bool takeaway_ = false;
    int patienceSeconds_ = 0;
    int groupId_ = 0;
    int groupSize_ = 1;

    StudentState state_ = StudentState::Queued;
    int assignedWindowId_ = -1;
    int queueEntryTime_ = 0;
    int accumulatedQueueWait_ = 0;
    int serviceStartTime_ = -1;
    int serviceEndTime_ = -1;
    int seatWaitStartTime_ = -1;
    int diningStartTime_ = -1;
    int finishedTime_ = -1;
    int droppedTime_ = -1;
};

std::string toString(StudentState state);

} // namespace bdss::core
