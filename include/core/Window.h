#pragma once

#include "core/Student.h"

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace bdss::core {

class Window {
public:
    Window(int id, std::string name, std::string category, double efficiency = 1.0);

    int getId() const { return id_; }
    const std::string& getName() const { return name_; }
    const std::string& getCategory() const { return category_; }
    double getEfficiency() const { return efficiency_; }
    std::size_t getQueueLength() const { return queue_.size(); }
    int getServedCount() const { return servedCount_; }
    int getDroppedCount() const { return droppedCount_; }
    bool empty() const { return queue_.empty() && !currentStudent_; }
    bool hasCurrentStudent() const { return static_cast<bool>(currentStudent_); }
    int getRemainingServiceTime() const { return remainingServiceTime_; }
    std::shared_ptr<Student> getCurrentStudent() const { return currentStudent_; }
    std::vector<std::shared_ptr<Student>> getQueuedStudents() const;

    void setProfile(std::string name, std::string category, double efficiency);
    std::vector<std::shared_ptr<Student>> releaseQueuedStudents();
    std::shared_ptr<Student> releaseCurrentStudent();

    void enqueue(const std::shared_ptr<Student>& student, int currentTime);
    std::shared_ptr<Student> tick(int currentTime);
    std::vector<std::shared_ptr<Student>> removeExpiredQueuedStudents(int currentTime);
    std::shared_ptr<Student> popLongestWaitingQueuedStudent();

private:
    void startNextIfNeeded(int currentTime);

    int id_ = 0;
    std::string name_;
    std::string category_;
    double efficiency_ = 1.0;
    std::deque<std::shared_ptr<Student>> queue_;
    std::shared_ptr<Student> currentStudent_;
    int remainingServiceTime_ = 0;
    int servedCount_ = 0;
    int droppedCount_ = 0;
};

} // namespace bdss::core
