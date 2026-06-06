#include "gui/RenderWidget.h"

#include <QFont>
#include <QLinearGradient>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <algorithm>
#include <array>
#include <cmath>

namespace bdss::gui {

RenderWidget::RenderWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(460);
    setAutoFillBackground(false);
}

void RenderWidget::setEngine(const bdss::core::SimulationEngine* engine) {
    engine_ = engine;
    update();
}

void RenderWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor("#F8FAFC"));

    const QRect content = rect().adjusted(12, 12, -12, -12);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawRoundedRect(content, 20, 20);

    if (!engine_) {
        drawEmptyState(painter, content);
        return;
    }

    const int leftWidth = std::max(300, width() / 3);
    const QRect windowsRect(content.left() + 18, content.top() + 18, leftWidth - 30, content.height() - 58);
    const QRect seatsRect(windowsRect.right() + 22, content.top() + 18,
                          content.right() - windowsRect.right() - 40, content.height() - 58);
    drawWindows(painter, windowsRect);
    drawSeats(painter, seatsRect);
    drawLegend(painter, QRect(content.left() + 18, content.bottom() - 30, content.width() - 36, 24));
}

void RenderWidget::drawEmptyState(QPainter& painter, const QRect& rect) {
    painter.setPen(QColor("#64748B"));
    painter.setFont(QFont(QStringLiteral("Sans Serif"), 13, QFont::DemiBold));
    painter.drawText(rect, Qt::AlignCenter, QStringLiteral("点击“开始”或“应用参数”后显示实时食堂状态"));
}

void RenderWidget::drawWindows(QPainter& painter, const QRect& rect) {
    painter.setFont(QFont(QStringLiteral("Sans Serif"), 11, QFont::DemiBold));
    painter.setPen(QColor("#0F172A"));
    painter.drawText(QRect(rect.left(), rect.top(), rect.width(), 24), Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("餐口排队"));

    const auto windows = engine_->getWindowSnapshots();
    if (windows.empty()) {
        return;
    }
    const int cardGap = 8;
    const int cardHeight = 54;
    int y = rect.top() + 34;
    const int maxVisible = std::max(1, (rect.height() - 34) / (cardHeight + cardGap));
    const int count = std::min(static_cast<int>(windows.size()), maxVisible);
    const int maxQueue = std::max(1, engine_->getTotalQueueLength());

    for (int i = 0; i < count; ++i) {
        const auto& w = windows[static_cast<std::size_t>(i)];
        QRect card(rect.left(), y, rect.width(), cardHeight);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#F1F5F9"));
        painter.drawRoundedRect(card, 12, 12);

        const double ratio = std::clamp(static_cast<double>(w.queueLength) / std::max(1, maxQueue), 0.0, 1.0);
        QRect bar(card.left(), card.bottom() - 8, static_cast<int>(card.width() * ratio), 8);
        painter.setBrush(QColor("#3B82F6"));
        painter.drawRoundedRect(bar, 4, 4);

        painter.setPen(QColor("#0F172A"));
        painter.setFont(QFont(QStringLiteral("Sans Serif"), 10, QFont::DemiBold));
        painter.drawText(card.adjusted(12, 4, -12, -28), Qt::AlignLeft | Qt::AlignVCenter,
                         QString::fromStdString(w.name) + QStringLiteral(" · ") + QString::fromStdString(w.category));
        painter.setFont(QFont(QStringLiteral("Sans Serif"), 9));
        painter.setPen(QColor("#475569"));
        const QString detail = QStringLiteral("队列 %1  |  效率 %2  |  已服务 %3%4")
                                   .arg(w.queueLength)
                                   .arg(w.efficiency, 0, 'f', 2)
                                   .arg(w.servedCount)
                                   .arg(w.serving ? QStringLiteral("  |  服务中") : QString());
        painter.drawText(card.adjusted(12, 24, -12, -8), Qt::AlignLeft | Qt::AlignVCenter, detail);
        y += cardHeight + cardGap;
    }

    if (static_cast<int>(windows.size()) > count) {
        painter.setPen(QColor("#64748B"));
        painter.drawText(QRect(rect.left(), y, rect.width(), 20), Qt::AlignCenter,
                         QStringLiteral("还有 %1 个餐口，增大窗口可查看更多").arg(static_cast<int>(windows.size()) - count));
    }
}

void RenderWidget::drawSeats(QPainter& painter, const QRect& rect) {
    painter.setFont(QFont(QStringLiteral("Sans Serif"), 11, QFont::DemiBold));
    painter.setPen(QColor("#0F172A"));
    painter.drawText(QRect(rect.left(), rect.top(), rect.width(), 24), Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("座位矩阵"));

    const auto snapshots = engine_->getSeatSnapshots();
    const auto& config = engine_->getConfig();
    const int rows = std::max(1, config.tableRows);
    const int cols = std::max(1, config.tableCols);
    const QRect gridRect = rect.adjusted(0, 36, 0, -4);
    const int cell = std::max(8, std::min(gridRect.width() / cols, gridRect.height() / rows));
    const int gridW = cell * cols;
    const int gridH = cell * rows;
    const int x0 = gridRect.left() + (gridRect.width() - gridW) / 2;
    const int y0 = gridRect.top() + (gridRect.height() - gridH) / 2;

    for (const auto& seat : snapshots) {
        const QRect cellRect(x0 + seat.col * cell + 1, y0 + seat.row * cell + 1, cell - 2, cell - 2);
        QColor color("#E2E8F0");
        if (seat.state == bdss::core::SeatState::Occupied) {
            if (seat.typeName == "本科生") color = QColor("#3B82F6");
            else if (seat.typeName == "研究生") color = QColor("#F97316");
            else color = QColor("#10B981");
        } else if (seat.state == bdss::core::SeatState::Cleaning) {
            color = QColor("#94A3B8");
        }
        painter.setPen(QPen(QColor("#CBD5E1"), 1));
        painter.setBrush(color);
        painter.drawRoundedRect(cellRect, std::max(2, cell / 6), std::max(2, cell / 6));
    }
}

void RenderWidget::drawLegend(QPainter& painter, const QRect& rect) {
    struct Item { QString name; QColor color; };
    const std::array<Item, 5> items{{
        {QStringLiteral("空座"), QColor("#E2E8F0")},
        {QStringLiteral("本科生"), QColor("#3B82F6")},
        {QStringLiteral("研究生"), QColor("#F97316")},
        {QStringLiteral("教职工"), QColor("#10B981")},
        {QStringLiteral("清洁中"), QColor("#94A3B8")}
    }};
    int x = rect.left();
    painter.setFont(QFont(QStringLiteral("Sans Serif"), 9));
    for (const auto& item : items) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(item.color);
        painter.drawRoundedRect(QRect(x, rect.center().y() - 6, 12, 12), 3, 3);
        painter.setPen(QColor("#475569"));
        painter.drawText(QRect(x + 18, rect.top(), 72, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, item.name);
        x += 86;
    }
}

} // namespace bdss::gui
