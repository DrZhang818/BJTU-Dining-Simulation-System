#pragma once

#include "core/Student.h"

#include <memory>
#include <queue>

namespace bdss::core {

class Window {
public:
    Window(int id, double efficiencyFactor = 1.0);

    void enqueue(const std::shared_ptr<Student>& student, int currentTime);
    std::shared_ptr<Student> tick(int currentTime);

    std::size_t getQueueLength() const { return queue_.size(); }
    int getId() const { return id_; }
    double getEfficiency() const { return efficiency_; }
    bool empty() const { return queue_.empty(); }

private:
    int id_;
    double efficiency_;
    double workAccumulator_;
    std::queue<std::shared_ptr<Student>> queue_;
};

} // namespace bdss::core