#include "core/Window.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace bdss::core {

Window::Window(int id, double efficiency)
    : id_(id), efficiency_(std::max(0.1, efficiency)) {
    if (id <= 0) {
        throw std::invalid_argument("Window id must be positive");
    }
}

int Window::getId() const noexcept { return id_; }
double Window::getEfficiency() const noexcept { return efficiency_; }

std::size_t Window::getQueueLength() const noexcept {
    return queue_.size();
}

std::size_t Window::getWorkloadLength() const noexcept {
    return queue_.size() + (current_ ? 1U : 0U);
}

bool Window::empty() const noexcept {
    return queue_.empty() && !current_;
}

bool Window::isServing() const noexcept {
    return static_cast<bool>(current_);
}

void Window::enqueue(const std::shared_ptr<Student>& student, int now) {
    if (!student) {
        throw std::invalid_argument("cannot enqueue null student");
    }
    student->startQueuing(now);
    queue_.push(student);
}

std::shared_ptr<Student> Window::tick(int now) {
    if (!current_ && !queue_.empty()) {
        current_ = queue_.front();
        queue_.pop();
        current_->startServing(now);
    }

    if (!current_) {
        return nullptr;
    }

    if (current_->advanceServiceOneSecond()) {
        auto completed = current_;
        completed->finishServiceAndWaitForSeat(now + 1);
        current_.reset();
        return completed;
    }
    return nullptr;
}

} // namespace bdss::core
