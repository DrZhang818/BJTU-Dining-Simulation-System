#pragma once

#include "core/Student.h"

#include <cstddef>
#include <memory>
#include <queue>

namespace bdss::core {

class Window {
public:
    Window(int id, double efficiency = 1.0);

    int getId() const noexcept;
    double getEfficiency() const noexcept;
    std::size_t getQueueLength() const noexcept;
    std::size_t getWorkloadLength() const noexcept;
    bool empty() const noexcept;
    bool isServing() const noexcept;

    void enqueue(const std::shared_ptr<Student>& student, int now);
    std::shared_ptr<Student> tick(int now);

private:
    int id_;
    double efficiency_;
    std::queue<std::shared_ptr<Student>> queue_;
    std::shared_ptr<Student> current_;
};

} // namespace bdss::core
