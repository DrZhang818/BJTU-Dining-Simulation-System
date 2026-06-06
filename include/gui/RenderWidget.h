#pragma once

#include <QWidget>

namespace bdss::core {
class SimulationEngine;
} // namespace bdss::core

namespace bdss::gui {

class RenderWidget final : public QWidget {
    Q_OBJECT
public:
    explicit RenderWidget(QWidget* parent = nullptr);

    void setEngine(const bdss::core::SimulationEngine* engine);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    const bdss::core::SimulationEngine* engine_ = nullptr;
};

} // namespace bdss::gui
