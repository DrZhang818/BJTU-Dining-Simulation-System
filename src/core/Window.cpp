#include "core/Window.h"

namespace bdss::core {

Window::Window(int id, double efficiencyFactor)
    : id_(id), efficiency_(efficiencyFactor), workAccumulator_(0.0) {}

void Window::enqueue(const std::shared_ptr<Student>& student, int currentTime) {
    if (!student) {
        return;
    }

    student->startQueuing(currentTime);
    queue_.push(student);
}

std::shared_ptr<Student> Window::tick(int currentTime) {
    if (queue_.empty()) {
        return nullptr;
    }

    auto currentStudent = queue_.front();

    if (currentStudent->getState() == StudentState::Queuing) {
        currentStudent->startServing(currentTime);
    }

    workAccumulator_ += efficiency_;

    while (workAccumulator_ >= 1.0 && currentStudent->getRemainingServiceTime() > 0) {
        currentStudent->decrementServiceTime();
        workAccumulator_ -= 1.0;
    }

    if (currentStudent->getRemainingServiceTime() <= 0) {
        currentStudent->finishServiceAndWaitForSeat(currentTime + 1);
        queue_.pop();
        workAccumulator_ = 0.0;
        return currentStudent;
    }

    return nullptr;
}

} // namespace bdss::core