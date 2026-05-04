#pragma once

#include <QWidget>

#include <memory>

namespace bdss::core {
class SimulationEngine;
}

namespace bdss::gui {

// 自定义画布：左侧绘制各窗口排队条，右侧绘制餐桌矩阵热力图。
// 仅持有 engine 的弱引用，自身不修改仿真状态；调用方在每个 tick 后 update() 触发重绘。
class RenderWidget : public QWidget {
    Q_OBJECT

public:
    explicit RenderWidget(QWidget* parent = nullptr);

    void setEngine(const bdss::core::SimulationEngine* engine);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void paintIdle(QPainter& painter);
    void paintWindowsArea(QPainter& painter, const QRectF& area);
    void paintCanteenArea(QPainter& painter, const QRectF& area);

    const bdss::core::SimulationEngine* engine_ = nullptr;
};

} // namespace bdss::gui
