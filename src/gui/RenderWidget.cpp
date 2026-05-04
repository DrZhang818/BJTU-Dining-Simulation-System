#include "gui/RenderWidget.h"

#include "core/Canteen.h"
#include "core/SimulationEngine.h"

#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QRectF>
#include <QString>

#include <algorithm>
#include <cmath>

namespace bdss::gui {

namespace {

constexpr int kQueueRowHeight = 28;
constexpr int kQueueGap = 6;
constexpr int kQueuePersonWidth = 14;
constexpr int kQueuePersonGap = 2;

QColor lerpColor(const QColor& a, const QColor& b, double t) {
    t = std::clamp(t, 0.0, 1.0);
    return QColor(
        static_cast<int>(a.red()   + (b.red()   - a.red())   * t),
        static_cast<int>(a.green() + (b.green() - a.green()) * t),
        static_cast<int>(a.blue()  + (b.blue()  - a.blue())  * t)
    );
}

// 按队列长度上色：短=绿；中=黄；长=红
QColor queueHeatColor(int length, int maxLength) {
    if (maxLength <= 0) {
        return QColor(120, 200, 120);
    }
    const double ratio = std::clamp(static_cast<double>(length) / static_cast<double>(maxLength), 0.0, 1.0);
    if (ratio < 0.5) {
        return lerpColor(QColor(120, 200, 120), QColor(255, 200, 60), ratio * 2.0);
    }
    return lerpColor(QColor(255, 200, 60), QColor(220, 60, 60), (ratio - 0.5) * 2.0);
}

} // namespace

RenderWidget::RenderWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(800, 400);
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor("#F8FAFC"));
    setPalette(pal);
}

void RenderWidget::setEngine(const bdss::core::SimulationEngine* engine) {
    engine_ = engine;
    update();
}

void RenderWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#F8FAFC"));

    if (!engine_) {
        paintIdle(painter);
        return;
    }

    const QRectF full(rect());
    const qreal margin = 12.0;
    const qreal leftWidth = full.width() * 0.42;

    QRectF leftArea(full.left() + margin, full.top() + margin,
                    leftWidth - margin * 1.5, full.height() - margin * 2);
    QRectF rightArea(leftArea.right() + margin, full.top() + margin,
                     full.right() - leftArea.right() - margin * 2, full.height() - margin * 2);

    paintWindowsArea(painter, leftArea);
    paintCanteenArea(painter, rightArea);
}

void RenderWidget::paintIdle(QPainter& painter) {
    painter.setPen(QColor("#94A3B8"));
    QFont font = painter.font();
    font.setPointSize(14);
    painter.setFont(font);
    painter.drawText(rect(), Qt::AlignCenter,
                     QStringLiteral("尚未启动仿真。\n请到【参数配置】页设置参数后点击【开始】。"));
}

void RenderWidget::paintWindowsArea(QPainter& painter, const QRectF& area) {
    painter.save();

    // 标题
    painter.setPen(QColor("#0F172A"));
    QFont title = painter.font();
    title.setPointSize(11);
    title.setBold(true);
    painter.setFont(title);
    painter.drawText(QRectF(area.left(), area.top(), area.width(), 22),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("窗口排队（左→右：队首→队尾）"));

    const auto queues = engine_->getWindowQueueLengths();
    const auto effs = engine_->getWindowEfficiencies();
    const int n = static_cast<int>(queues.size());
    if (n == 0) {
        painter.restore();
        return;
    }

    const int maxLen = std::max(1, *std::max_element(queues.begin(), queues.end()));

    const qreal contentTop = area.top() + 28.0;
    const qreal totalRowsHeight = (kQueueRowHeight + kQueueGap) * n;
    const qreal availH = area.height() - 28.0;
    const qreal rowH = (totalRowsHeight > availH)
        ? std::max<qreal>(14.0, (availH - kQueueGap * (n - 1)) / static_cast<qreal>(n))
        : kQueueRowHeight;

    QFont labelFont = painter.font();
    labelFont.setPointSize(9);
    labelFont.setBold(false);
    painter.setFont(labelFont);

    for (int i = 0; i < n; ++i) {
        const qreal y = contentTop + i * (rowH + kQueueGap);
        const QRectF labelRect(area.left(), y, 92.0, rowH);

        const QString label = QString(QStringLiteral("窗口 %1 (×%2)"))
                                  .arg(i + 1)
                                  .arg(QString::number(effs.size() > static_cast<std::size_t>(i)
                                                           ? effs[i] : 1.0, 'f', 1));
        painter.setPen(QColor("#334155"));
        painter.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, label);

        const qreal trackLeft = labelRect.right() + 6.0;
        const qreal trackWidth = area.right() - trackLeft - 50.0;
        const QRectF track(trackLeft, y + 2.0, trackWidth, rowH - 4.0);

        // 跑道底色
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#E2E8F0"));
        painter.drawRoundedRect(track, 4.0, 4.0);

        // 队列里的"人"用小方块
        const int queueLen = queues[i];
        if (queueLen > 0) {
            const QColor color = queueHeatColor(queueLen, maxLen);
            painter.setBrush(color);
            painter.setPen(QPen(color.darker(130), 0.5));

            const int maxFit = static_cast<int>(track.width() / (kQueuePersonWidth + kQueuePersonGap));
            const int drawCount = std::min(queueLen, std::max(1, maxFit));
            for (int j = 0; j < drawCount; ++j) {
                const qreal px = track.left() + 3.0 + j * (kQueuePersonWidth + kQueuePersonGap);
                const QRectF person(px, track.top() + 2.0,
                                    kQueuePersonWidth, track.height() - 4.0);
                painter.drawRoundedRect(person, 2.0, 2.0);
            }

            // 如果显示不下，画一个 "+N" 提示
            if (queueLen > drawCount) {
                painter.setPen(QColor("#1E293B"));
                painter.drawText(QRectF(track.right() - 60.0, track.top(), 56.0, track.height()),
                                 Qt::AlignRight | Qt::AlignVCenter,
                                 QString(QStringLiteral("+%1")).arg(queueLen - drawCount));
            }
        }

        // 数字标注队列长度
        painter.setPen(QColor("#0F172A"));
        QFont counterFont = painter.font();
        counterFont.setBold(true);
        painter.setFont(counterFont);
        painter.drawText(QRectF(track.right() + 6.0, y, 44.0, rowH),
                         Qt::AlignLeft | Qt::AlignVCenter, QString::number(queueLen));
        painter.setFont(labelFont);
    }

    painter.restore();
}

void RenderWidget::paintCanteenArea(QPainter& painter, const QRectF& area) {
    painter.save();

    const auto& canteen = engine_->getCanteen();
    const int rows = canteen.getRows();
    const int cols = canteen.getCols();

    painter.setPen(QColor("#0F172A"));
    QFont title = painter.font();
    title.setPointSize(11);
    title.setBold(true);
    painter.setFont(title);
    painter.drawText(QRectF(area.left(), area.top(), area.width(), 22),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QString(QStringLiteral("餐桌矩阵 %1 × %2 · 已就座 %3"))
                         .arg(rows).arg(cols).arg(canteen.getOccupiedSeats()));

    const qreal padTop = 28.0;
    const qreal contentW = area.width();
    const qreal contentH = area.height() - padTop;

    if (rows <= 0 || cols <= 0 || contentW <= 0 || contentH <= 0) {
        painter.restore();
        return;
    }

    const qreal cellW = contentW / static_cast<qreal>(cols);
    const qreal cellH = contentH / static_cast<qreal>(rows);
    const qreal cell = std::min(cellW, cellH);
    const qreal gridW = cell * cols;
    const qreal gridH = cell * rows;
    const qreal originX = area.left() + (contentW - gridW) / 2.0;
    const qreal originY = area.top() + padTop + (contentH - gridH) / 2.0;

    const auto snapshot = canteen.getSeatSnapshot();

    const QColor emptyColor("#E2E8F0");
    const QColor occupiedColor("#22C55E");
    const QColor edgeColor("#94A3B8");

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const QRectF rect(originX + c * cell + 1.0,
                              originY + r * cell + 1.0,
                              cell - 2.0, cell - 2.0);
            const bool occupied = (snapshot[r][c] != 0);
            painter.setPen(QPen(edgeColor, 0.5));
            painter.setBrush(occupied ? occupiedColor : emptyColor);
            painter.drawRoundedRect(rect, std::min<qreal>(4.0, cell * 0.2),
                                    std::min<qreal>(4.0, cell * 0.2));
        }
    }

    // 图例
    const qreal legendY = originY + gridH + 8.0;
    if (legendY + 18.0 < area.bottom()) {
        QFont legendFont = painter.font();
        legendFont.setPointSize(9);
        legendFont.setBold(false);
        painter.setFont(legendFont);

        painter.setPen(Qt::NoPen);
        painter.setBrush(emptyColor);
        painter.drawRoundedRect(QRectF(area.left(), legendY, 18.0, 14.0), 2.0, 2.0);
        painter.setPen(QColor("#475569"));
        painter.drawText(QRectF(area.left() + 22.0, legendY - 1, 80.0, 16.0),
                         Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("空座位"));

        painter.setPen(Qt::NoPen);
        painter.setBrush(occupiedColor);
        painter.drawRoundedRect(QRectF(area.left() + 110.0, legendY, 18.0, 14.0), 2.0, 2.0);
        painter.setPen(QColor("#475569"));
        painter.drawText(QRectF(area.left() + 132.0, legendY - 1, 80.0, 16.0),
                         Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("有人就餐"));
    }

    painter.restore();
}

} // namespace bdss::gui
