#pragma once

#include "core/SimulationEngine.h"

#include <QWidget>

class QPainter;

namespace bdss::gui {

class RenderWidget : public QWidget {
public:
    explicit RenderWidget(QWidget* parent = nullptr);
    void setEngine(const bdss::core::SimulationEngine* engine);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void drawEmptyState(QPainter& painter, const QRect& rect);
    void drawWindows(QPainter& painter, const QRect& rect);
    void drawSeats(QPainter& painter, const QRect& rect);
    void drawLegend(QPainter& painter, const QRect& rect);

    const bdss::core::SimulationEngine* engine_ = nullptr;
};

} // namespace bdss::gui
