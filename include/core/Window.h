#pragma once
#include <memory>
#include <queue>
// 前向声明
namespace bdss::core {
class Student;
}

namespace bdss::core {

class Window {
public:
    Window(int id, double efficiencyFactor = 1.0);

    // 将学生加入该窗口的排队队列
    void enqueue(std::shared_ptr<Student> student);

    // 核心推进逻辑：时间流逝 1 秒，处理队首学生
    // 传入 currentTime 以便在学生打完饭时记录状态转换的时间
    std::shared_ptr<Student> tick(int currentTime);

    // 获取当前排队人数
    size_t getQueueLength() const;

    // 获取窗口 ID
    int getId() const { return m_id; }

private:
    int m_id;                                      // 窗口编号
    double m_efficiency;                           // 窗口效率系数 (如 1.2)
    double m_workAccumulator;                      // 用于处理浮点型效率与整型耗时的转换积攒
    std::queue<std::shared_ptr<Student>> m_queue;  // 排队队列
};

}  // namespace bdss::core