#pragma

#include <format>
#include <optional>
#include <string>

namespace bdss::core {

enum class StudentState {
    Arrived,  // 学生到达食堂
    Queuing,  // 学生排队等待打饭
    Dining,   // 学生正在就餐
    Left      // 学生离开食堂
};

class Student {
   public:
    Student(int id, int arrivalTime, int targetServiceTime, int targetDiningTime);

    void startQueuing();
    void finishQueuingAndStartDining(int time);
    void finishDiningAndLeave(int time);

    int getId() const { return id_; }
    StudentState getState() const { return state_; }

    int getRemainingServiceTime() const { return remainingServiceTime_; }
    void decrementServiceTime() {
        if (remainingServiceTime_ > 0) { remainingServiceTime_--; }
    }

    int getWaitTime() const;
    int getTotalTime() const;

    std::string toString() const;

   private:
    int id_;
    StudentState state_;

    int remainingServiceTime_;  // 打饭剩余时间
    int remainingDiningTime_;   // 就餐剩余时间

    int arrivalTime_;                     // 到达时间
    std::optional<int> startDiningTime_;  // 开始就餐时间
    std::optional<int> leaveTime_;        // 离开时间
};

}  // namespace bdss::core