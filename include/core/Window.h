#pragma once

#include "core/Config.h"
#include "core/Student.h"

#include <cstddef>
#include <deque>
#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace bdss::core {

class Window {
public:
    Window(int id, double efficiency = 1.0);

    int getId() const noexcept { return id_; }
    double getEfficiency() const noexcept { return efficiency_; }
    std::size_t getQueueLength() const noexcept { return queue_.size(); }
    std::size_t getWorkloadLength() const noexcept { return queue_.size() + (current_ ? 1U : 0U); }
    bool empty() const noexcept { return queue_.empty() && !current_; }
    bool isServing() const noexcept { return static_cast<bool>(current_); }

    void enqueue(const std::shared_ptr<Student>& student, int now);
    void reEnqueue(const std::shared_ptr<Student>& student);
    std::shared_ptr<Student> tick(int now);

    std::vector<std::shared_ptr<Student>> collectImpatient();

    void setCategory(const std::string& category) noexcept { category_ = category; }
    const std::string& getCategory() const noexcept { return category_; }

private:
    int id_;
    double efficiency_;
    std::deque<std::shared_ptr<Student>> queue_;
    std::shared_ptr<Student> current_;
    std::string category_;
};

} // namespace bdss::core
