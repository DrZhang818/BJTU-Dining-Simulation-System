#include "gui/ChartWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPen>
#include <QFont>
#include <algorithm>

namespace bdss::gui {

ChartWidget::ChartWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(320);
    setAutoFillBackground(false);
}

void ChartWidget::setRecords(const std::vector<bdss::utils::TickRecord>& records) {
    records_ = records;
    update();
}

void ChartWidget::clear() {
    records_.clear();
    update();
}

void ChartWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor("#F8FAFC"));

    const QRect area = rect().adjusted(28, 28, -28, -48);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawRoundedRect(rect().adjusted(8, 8, -8, -8), 18, 18);

    painter.setPen(QPen(QColor("#CBD5E1"), 1));
    painter.drawRect(area);
    painter.setPen(QColor("#0F172A"));
    painter.setFont(QFont(QStringLiteral("Sans Serif"), 11, QFont::DemiBold));
    painter.drawText(QRect(0, 8, width(), 24), Qt::AlignCenter, QStringLiteral("仿真统计曲线"));

    if (records_.size() < 2) {
        painter.setPen(QColor("#64748B"));
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("开始仿真后将在这里显示排队、等座、座位利用率趋势"));
        return;
    }

    int maxPeople = 1;
    for (const auto& record : records_) {
        maxPeople = std::max(maxPeople, std::max(record.totalQueueLength, record.waitingForSeatCount));
    }
    maxPeople = std::max(maxPeople, 4);
    const int t0 = records_.front().time;
    const int t1 = std::max(records_.back().time, t0 + 1);

    auto pointPeople = [&](int time, int value) {
        const double x = area.left() + (static_cast<double>(time - t0) / (t1 - t0)) * area.width();
        const double y = area.bottom() - (static_cast<double>(value) / maxPeople) * area.height();
        return QPointF(x, y);
    };
    auto pointUtil = [&](int time, double value) {
        const double x = area.left() + (static_cast<double>(time - t0) / (t1 - t0)) * area.width();
        const double y = area.bottom() - std::clamp(value, 0.0, 1.0) * area.height();
        return QPointF(x, y);
    };

    painter.setFont(QFont(QStringLiteral("Sans Serif"), 8));
    painter.setPen(QPen(QColor("#E2E8F0"), 1));
    for (int i = 1; i < 5; ++i) {
        const int y = area.top() + i * area.height() / 5;
        painter.drawLine(area.left(), y, area.right(), y);
    }

    int legendIndex = 0;
    auto drawPath = [&](auto getter, const QColor& color, const QString& label) {
        QPainterPath path;
        bool first = true;
        for (const auto& record : records_) {
            const QPointF point = getter(record);
            if (first) {
                path.moveTo(point);
                first = false;
            } else {
                path.lineTo(point);
            }
        }
        painter.setPen(QPen(color, 2.2));
        painter.drawPath(path);
        painter.setPen(color);
        painter.drawText(QPointF(area.left() + 12, area.top() + 20 + 18 * legendIndex), label);
        ++legendIndex;
    };

    drawPath([&](const auto& r) { return pointPeople(r.time, r.totalQueueLength); }, QColor("#EF4444"), QStringLiteral("排队总人数"));
    drawPath([&](const auto& r) { return pointPeople(r.time, r.waitingForSeatCount); }, QColor("#F59E0B"), QStringLiteral("等座人数"));
    drawPath([&](const auto& r) { return pointUtil(r.time, r.seatUtilization); }, QColor("#10B981"), QStringLiteral("座位利用率"));

    painter.setPen(QColor("#64748B"));
    painter.drawText(QRect(area.left(), area.bottom() + 8, area.width(), 18), Qt::AlignCenter,
                     QStringLiteral("仿真时间 / 秒"));
    painter.save();
    painter.translate(12, area.center().y());
    painter.rotate(-90);
    painter.drawText(QRect(-area.height() / 2, 0, area.height(), 18), Qt::AlignCenter, QStringLiteral("人数 / 利用率"));
    painter.restore();
}

} // namespace bdss::gui
