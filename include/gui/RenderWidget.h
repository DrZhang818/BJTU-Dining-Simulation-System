#pragma once

#include "core/SimulationEngine.h"

#include <QWidget>

class QWheelEvent;

class QPainter;

namespace bdss::gui {

class RenderWidget : public QWidget {
public:
    explicit RenderWidget(QWidget* parent = nullptr);
    void setEngine(const bdss::core::SimulationEngine* engine);

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void drawEmptyState(QPainter& painter, const QRect& rect);
    void drawWindows(QPainter& painter, const QRect& rect);
    void drawSeats(QPainter& painter, const QRect& rect);
    void drawLegend(QPainter& painter, const QRect& rect);

    const bdss::core::SimulationEngine* engine_ = nullptr;
    int windowScrollOffset_ = 0;
};

} // namespace bdss::gui
