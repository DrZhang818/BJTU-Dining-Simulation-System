#include "core/Window.h"

#include <algorithm>

namespace bdss::core {

Window::Window(int id) : id_(id) {}

int Window::id() const {
    return id_;
}

void Window::enqueue(const Student& student) {
    queue_.push_back(student);
}

ServiceResult Window::tick(int currentTime, int serviceDurationSeconds) {
    ServiceResult result;

    if (current_) {
        --remainingServiceSeconds_;
        if (remainingServiceSeconds_ <= 0) {
            current_->serviceEndTime = currentTime;
            current_->state = StudentState::WaitingForSeat;
            result.hasStudent = true;
            result.student = *current_;
            current_.reset();
            ++servedCount_;
        }
    }

    if (!current_ && !queue_.empty()) {
        current_ = queue_.front();
        queue_.pop_front();
        current_->state = StudentState::Serving;
        current_->serviceStartTime = currentTime;
        remainingServiceSeconds_ = std::max(1, serviceDurationSeconds);
    }

    return result;
}

int Window::queueLength() const {
    return static_cast<int>(queue_.size());
}

int Window::servedCount() const {
    return servedCount_;
}

bool Window::busy() const {
    return current_.has_value();
}

} // namespace bdss::core
