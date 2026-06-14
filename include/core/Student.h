#pragma once

namespace bdss::core {

enum class StudentState {
    Queuing,
    Serving,
    WaitingForSeat,
    Dining,
    Finished,
    Dropped
};

struct Student {
    int id = 0;
    int arrivalTime = 0;
    int serviceStartTime = -1;
    int serviceEndTime = -1;
    int seatWaitStartTime = -1;
    int seatStartTime = -1;
    bool takeaway = false;
    StudentState state = StudentState::Queuing;
};

} // namespace bdss::core
