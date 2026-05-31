#include "gui/RenderWidget.h"

#include <QPainter>

#include <algorithm>

namespace bdss::gui {

RenderWidget::RenderWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(360, 360);
}

void RenderWidget::setSeatMatrix(int rows, int cols, int occupiedSeats) {
    rows_ = std::max(1, rows);
    cols_ = std::max(1, cols);
    occupiedSeats_ = std::clamp(occupiedSeats, 0, rows_ * cols_);
    update();
}

void RenderWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(250, 250, 250));

    const int margin = 16;
    const int cellW = std::max(4, (width() - margin * 2) / cols_);
    const int cellH = std::max(4, (height() - margin * 2) / rows_);
    int index = 0;
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            const QRect cell(margin + c * cellW + 2, margin + r * cellH + 2, cellW - 4, cellH - 4);
            const bool occupied = index < occupiedSeats_;
            painter.setPen(QColor(80, 80, 80));
            painter.setBrush(occupied ? QColor(80, 130, 220) : QColor(230, 235, 245));
            painter.drawRoundedRect(cell, 4, 4);
            ++index;
        }
    }
}

} // namespace bdss::gui
