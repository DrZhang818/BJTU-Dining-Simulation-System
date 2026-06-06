#pragma once

#include "core/Config.h"

#include <algorithm>
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

    int getId() const noexcept { return id_; }
    StudentState getState() const noexcept { return state_; }

    int getArrivalTime() const noexcept { return arrivalTime_; }
    int getQueueStartTime() const noexcept { return queueStartTime_; }
    int getServiceStartTime() const noexcept { return serviceStartTime_; }
    int getServiceEndTime() const noexcept { return serviceEndTime_; }
    int getDiningStartTime() const noexcept { return diningStartTime_; }
    int getLeaveTime() const noexcept { return leaveTime_; }
    int getServiceTime() const noexcept { return serviceTime_; }
    int getDiningTime() const noexcept { return diningTime_; }
    int getRemainingServiceTime() const noexcept { return remainingServiceTime_; }
    int getRemainingDiningTime() const noexcept { return remainingDiningTime_; }
    int getSeatRow() const noexcept { return seatRow_; }
    int getSeatCol() const noexcept { return seatCol_; }

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

    // ---- 偏好系统 (A) ----
    void setProfile(ProfileType p) noexcept { profile_ = p; }
    ProfileType getProfile() const noexcept { return profile_; }
    void setPreferredCategory(int c) noexcept { preferredCategory_ = c; }
    int getPreferredCategory() const noexcept { return preferredCategory_; }

    // ---- 排队耐心 (C) ----
    void setPatience(int seconds) noexcept { patienceLeft_ = seconds; }
    bool isImpatient() const noexcept { return patienceLeft_ <= 0; }
    void tickPatience() noexcept { if (patienceLeft_ > 0) --patienceLeft_; }

    // ---- 打包/堂食 (D) ----
    void setTakeaway(bool t) noexcept { isTakeaway_ = t; }
    bool isTakeaway() const noexcept { return isTakeaway_; }

    // ---- 结伴就餐 (E) ----
    void setGroupId(int g) noexcept { groupId_ = g; }
    int getGroupId() const noexcept { return groupId_; }
    bool hasGroup() const noexcept { return groupId_ >= 0; }

    // ---- 服务窗口 (G) ----
    void setServedByWindowId(int id) noexcept { servedByWindowId_ = id; }
    int getServedByWindowId() const noexcept { return servedByWindowId_; }

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

    ProfileType profile_ = ProfileType::Undergrad;
    int preferredCategory_ = -1;
    int patienceLeft_ = 60;       // 剩余耐心（秒），默认 60
    bool isTakeaway_ = false;
    int groupId_ = -1;
    int servedByWindowId_ = -1;
};

std::ostream& operator<<(std::ostream& os, const Student& student);

} // namespace bdss::core
