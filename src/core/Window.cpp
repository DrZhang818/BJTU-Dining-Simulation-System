#include "core/Window.h"

#include "core/Student.h"

namespace bdss::core {

Window::Window(int id, double efficiencyFactor)
    : m_id(id), m_efficiency(efficiencyFactor), m_workAccumulator(0.0) {}

void Window::enqueue(std::shared_ptr<Student> student) {
    if (student) {
        student->startQueuing();
        m_queue.push(student);
    }
}

std::shared_ptr<Student> Window::tick(int currentTime) {
    if (m_queue.empty()) {
        return nullptr;  // 没人在排队
    }

    auto currentStudent = m_queue.front();

    // 累积工作量，根据效率系数增加
    m_workAccumulator += m_efficiency;

    // 消耗积攒的工作量来推动学生的打饭进度
    while (m_workAccumulator >= 1.0 && currentStudent->getRemainingServiceTime() > 0) {
        currentStudent->decrementServiceTime();  // 打饭进度推进
        m_workAccumulator -= 1.0;
    }

    // 判断队首学生是否打饭完毕
    if (currentStudent->getRemainingServiceTime() <= 0) {
        // 打完饭，记录当前时间，更新学生状态为 Dining
        currentStudent->finishQueuingAndStartDining(currentTime);

        m_queue.pop();            // 出队
        m_workAccumulator = 0.0;  // 重置累加器，准备迎接下一个学生
        return currentStudent;    // 返回给 Canteen 找座位
    }

    return nullptr;  // 还没打完，继续留在队伍首位
}

size_t Window::getQueueLength() const { return m_queue.size(); }

}  // namespace bdss::core