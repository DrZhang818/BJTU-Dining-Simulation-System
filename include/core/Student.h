#pragma once

#include <iosfwd>
#include <string>

namespace bdss::core {

enum class StudentState {
    Arrived,
    Queuing,
    Serving,
    WaitingForSeat,
    Dining,
    Left
};

std::string toString(StudentState state);

class Student {
public:
    Student(int id, int arrivalTime, int serviceTime, int diningTime);

    int getId() const noexcept;
    StudentState getState() const noexcept;

    int getArrivalTime() const noexcept;
    int getQueueStartTime() const noexcept;
    int getServiceStartTime() const noexcept;
    int getServiceEndTime() const noexcept;
    int getDiningStartTime() const noexcept;
    int getLeaveTime() const noexcept;

    int getServiceTime() const noexcept;
    int getDiningTime() const noexcept;
    int getRemainingServiceTime() const noexcept;
    int getRemainingDiningTime() const noexcept;
    int getSeatRow() const noexcept;
    int getSeatCol() const noexcept;

    void setServiceTime(int seconds);
    void startQueuing(int now);
    void startServing(int now);
    bool advanceServiceOneSecond();
    void finishServiceAndWaitForSeat(int now);
    void startDining(int now, int row, int col);
    bool advanceDiningOneSecond();
    void finishDiningAndLeave(int now);

    int getQueueWaitTime() const noexcept;
    int getSeatWaitTime() const noexcept;
    int getActualServiceTime() const noexcept;
    int getActualDiningTime() const noexcept;
    int getTotalTime() const noexcept;

private:
    int id_;
    int arrivalTime_;
    int serviceTime_;
    int diningTime_;
    int remainingServiceTime_;
    int remainingDiningTime_;
    StudentState state_ = StudentState::Arrived;

    int queueStartTime_ = -1;
    int serviceStartTime_ = -1;
    int serviceEndTime_ = -1;
    int diningStartTime_ = -1;
    int leaveTime_ = -1;
    int seatRow_ = -1;
    int seatCol_ = -1;
};

std::ostream& operator<<(std::ostream& os, const Student& student);

} // namespace bdss::core
