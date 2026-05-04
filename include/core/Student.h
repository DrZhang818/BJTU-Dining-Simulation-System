#pragma once

#include <optional>
#include <string>

namespace bdss::core {

enum class StudentState {
    Arrived,        // 刚到达食堂
    Queuing,        // 正在窗口排队
    Serving,        // 正在窗口打饭
    WaitingForSeat, // 打完饭，正在等待空座
    Dining,         // 正在就餐
    Left            // 已离开食堂
};

class Student {
public:
    Student(int id, int arrivalTime, int targetServiceTime, int targetDiningTime);

    void startQueuing(int time);
    void startServing(int time);
    void finishServiceAndWaitForSeat(int time);
    void startDining(int time);
    void finishDiningAndLeave(int time);

    int getId() const { return id_; }
    StudentState getState() const { return state_; }

    int getArrivalTime() const { return arrivalTime_; }
    int getRemainingServiceTime() const { return remainingServiceTime_; }
    int getRemainingDiningTime() const { return remainingDiningTime_; }

    void decrementServiceTime();
    void decrementDiningTime();

    int getWaitTime() const;        // 从到达到开始打饭的等待时间
    int getSeatWaitTime() const;    // 从打完饭到开始就餐的等待时间
    int getServiceTime() const;     // 实际打饭耗时
    int getTotalTime() const;       // 从到达到离开的总时间

    std::string toString() const;
    static std::string stateToString(StudentState state);

private:
    int id_;
    StudentState state_;

    int targetServiceTime_;
    int targetDiningTime_;
    int remainingServiceTime_;
    int remainingDiningTime_;

    int arrivalTime_;
    std::optional<int> queueStartTime_;
    std::optional<int> serviceStartTime_;
    std::optional<int> serviceEndTime_;
    std::optional<int> diningStartTime_;
    std::optional<int> leaveTime_;
};

} // namespace bdss::core