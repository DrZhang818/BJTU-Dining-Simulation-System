#pragma once

#include "core/Student.h"

#include <deque>
#include <optional>

namespace bdss::core {

struct ServiceResult {
    bool hasStudent = false;
    Student student;
};

class Window {
public:
    explicit Window(int id = 0);

    int id() const;
    void enqueue(const Student& student);
    ServiceResult tick(int currentTime, int serviceDurationSeconds);

    int queueLength() const;
    int servedCount() const;
    bool busy() const;

private:
    int id_ = 0;
    std::deque<Student> queue_;
    std::optional<Student> current_;
    int remainingServiceSeconds_ = 0;
    int servedCount_ = 0;
};

} // namespace bdss::core
