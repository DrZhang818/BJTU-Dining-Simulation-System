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

void Window::enqueue(const std::shared_ptr<Student>& student, int now) {
    if (!student) {
        throw std::invalid_argument("cannot enqueue null student");
    }
    student->startQueuing(now);
    queue_.push_back(student);
}

void Window::reEnqueue(const std::shared_ptr<Student>& student) {
    queue_.push_back(student);
}

std::shared_ptr<Student> Window::tick(int now) {
    if (!current_ && !queue_.empty()) {
        current_ = queue_.front();
        queue_.pop_front();
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

std::vector<std::shared_ptr<Student>> Window::collectImpatient() {
    std::vector<std::shared_ptr<Student>> impatient;
    auto it = queue_.begin();
    while (it != queue_.end()) {
        auto& student = *it;
        student->tickPatience();
        if (student->isImpatient()) {
            impatient.push_back(student);
            it = queue_.erase(it);
        } else {
            ++it;
        }
    }
    return impatient;
}

} // namespace bdss::core
