#include "core/Window.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace bdss::core {

Window::Window(int id, std::string name, std::string category, double efficiency)
    : id_(id), name_(std::move(name)), category_(std::move(category)), efficiency_(std::max(0.05, efficiency)) {}

void Window::enqueue(const std::shared_ptr<Student>& student, int currentTime) {
    if (!student) {
        return;
    }
    student->enterQueue(currentTime, id_);
    queue_.push_back(student);
}

std::vector<std::shared_ptr<Student>> Window::getQueuedStudents() const {
    return {queue_.begin(), queue_.end()};
}

void Window::setProfile(std::string name, std::string category, double efficiency) {
    name_ = std::move(name);
    category_ = std::move(category);
    efficiency_ = std::max(0.05, efficiency);
}

std::vector<std::shared_ptr<Student>> Window::releaseQueuedStudents() {
    std::vector<std::shared_ptr<Student>> students;
    students.reserve(queue_.size());
    while (!queue_.empty()) {
        students.push_back(queue_.front());
        queue_.pop_front();
    }
    return students;
}

std::shared_ptr<Student> Window::releaseCurrentStudent() {
    auto student = currentStudent_;
    currentStudent_.reset();
    remainingServiceTime_ = 0;
    return student;
}

std::shared_ptr<Student> Window::tick(int currentTime) {
    startNextIfNeeded(currentTime);
    if (!currentStudent_) {
        return nullptr;
    }

    --remainingServiceTime_;
    if (remainingServiceTime_ <= 0) {
        auto completed = currentStudent_;
        completed->finishService(currentTime + 1);
        currentStudent_.reset();
        remainingServiceTime_ = 0;
        ++servedCount_;
        startNextIfNeeded(currentTime + 1);
        return completed;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Student>> Window::removeExpiredQueuedStudents(int currentTime) {
    std::vector<std::shared_ptr<Student>> expired;
    auto it = queue_.begin();
    while (it != queue_.end()) {
        if ((*it)->isPatienceExpired(currentTime)) {
            expired.push_back(*it);
            it = queue_.erase(it);
        } else {
            ++it;
        }
    }
    return expired;
}

std::shared_ptr<Student> Window::popLongestWaitingQueuedStudent() {
    if (queue_.empty()) {
        return nullptr;
    }
    auto student = queue_.front();
    queue_.pop_front();
    return student;
}

void Window::startNextIfNeeded(int currentTime) {
    if (currentStudent_ || queue_.empty()) {
        return;
    }
    currentStudent_ = queue_.front();
    queue_.pop_front();
    currentStudent_->startService(currentTime);
    remainingServiceTime_ = std::max(1, static_cast<int>(std::ceil(currentStudent_->getServiceTime() / efficiency_)));
}

} // namespace bdss::core
