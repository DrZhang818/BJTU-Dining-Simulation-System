#include "gui/RenderWidget.h"

#include "core/Canteen.h"
#include "core/Config.h"
#include "core/SimulationEngine.h"
#include "core/Student.h"
#include "core/Window.h"

#include <QFont>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace bdss::gui {

namespace {

constexpr int kQueueRowHeight = 26;
constexpr int kQueueGap = 4;
constexpr int kQueuePersonWidth = 12;
constexpr int kQueuePersonGap = 2;

// 画像颜色
QColor profileColor(core::ProfileType type) {
    switch (type) {
    case core::ProfileType::Undergrad: return QColor(66, 133, 244);   // 蓝色
    case core::ProfileType::Grad:      return QColor(52, 168, 83);    // 绿色
    case core::ProfileType::Staff:     return QColor(234, 134, 43);   // 橙色
    }
    return QColor(120, 120, 120);
}

QColor queueHeatColor(int length, int maxLength) {
    if (maxLength <= 0) return QColor(120, 200, 120);
    const double ratio = std::clamp(static_cast<double>(length) / static_cast<double>(maxLength), 0.0, 1.0);
    const auto lerp = [](const QColor& a, const QColor& b, double t) {
        t = std::clamp(t, 0.0, 1.0);
        return QColor(
            static_cast<int>(a.red()   + (b.red()   - a.red())   * t),
            static_cast<int>(a.green() + (b.green() - a.green()) * t),
            static_cast<int>(a.blue()  + (b.blue()  - a.blue())  * t)
        );
    };
    if (ratio < 0.5) return lerp(QColor(120, 200, 120), QColor(255, 200, 60), ratio * 2.0);
    return lerp(QColor(255, 200, 60), QColor(220, 60, 60), (ratio - 0.5) * 2.0);
}

} // namespace

RenderWidget::RenderWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(600, 320);
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor("#F8FAFC"));
    setPalette(pal);
}

void RenderWidget::setEngine(const bdss::core::SimulationEngine* engine) {
    engine_ = engine;
    update();
}

void RenderWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#F8FAFC"));

    if (!engine_) {
        painter.setPen(QColor("#94A3B8"));
        QFont f = painter.font();
        f.setPointSize(13);
        painter.setFont(f);
        painter.drawText(rect(), Qt::AlignCenter,
                         QStringLiteral("尚未启动仿真。\n请到参数配置页设置参数后点击「开始」。"));
        return;
    }

    const QRectF full(rect());
    constexpr double margin = 10.0;
    const double leftW = full.width() * 0.40;

    const QRectF left(full.left() + margin, full.top() + margin,
                      leftW - margin * 1.5, full.height() - margin * 2);
    const QRectF right(left.right() + margin, full.top() + margin,
                       full.right() - left.right() - margin * 2, full.height() - margin * 2);

    // 左：窗口排队
    painter.save();
    painter.setPen(QColor("#0F172A"));
    QFont titleF = painter.font();
    titleF.setPointSize(10);
    titleF.setBold(true);
    painter.setFont(titleF);
    painter.drawText(QRectF(left.left(), left.top(), left.width(), 20),
                     Qt::AlignLeft, QStringLiteral("窗口排队（各窗口长度）"));

    const auto& windows = engine_->windows();
    const int nWin = static_cast<int>(windows.size());
    const bool hasPref = engine_->config().enablePreferences && !engine_->config().windowCategories.empty();

    if (nWin > 0) {
        std::vector<int> lengths;
        lengths.reserve(static_cast<std::size_t>(nWin));
        for (const auto& w : windows) {
            lengths.push_back(static_cast<int>(w.getQueueLength()));
        }
        const int maxLen = std::max(1, *std::max_element(lengths.begin(), lengths.end()));
        const double contentTop = left.top() + 24.0;
        const double rowH = std::min<double>(kQueueRowHeight,
            (left.height() - 24.0 - static_cast<double>(nWin - 1) * kQueueGap) / static_cast<double>(nWin));

        QFont labelF = painter.font();
        labelF.setBold(false);
        labelF.setPointSize(8);
        painter.setFont(labelF);

        for (int i = 0; i < nWin; ++i) {
            const double y = contentTop + i * (rowH + kQueueGap);
            QString label;
            if (hasPref) {
                label = QString("W%1(%2)").arg(i + 1).arg(QString::fromStdString(windows[static_cast<std::size_t>(i)].getCategory()));
            } else {
                label = QString("W%1").arg(i + 1);
            }

            painter.setPen(QColor("#334155"));
            painter.drawText(QRectF(left.left(), y, 72.0, rowH), Qt::AlignLeft | Qt::AlignVCenter, label);

            const double trackL = left.left() + 76.0;
            const double trackW = left.right() - trackL - 40.0;
            const QRectF track(trackL, y + 1.0, trackW, rowH - 2.0);

            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor("#E2E8F0"));
            painter.drawRoundedRect(track, 3.0, 3.0);

            const int qLen = lengths[i];
            if (qLen > 0) {
                const QColor c = queueHeatColor(qLen, maxLen);
                painter.setBrush(c);
                painter.setPen(QPen(c.darker(130), 0.5));
                const int maxFit = std::max(1, static_cast<int>(track.width() / (kQueuePersonWidth + kQueuePersonGap)));
                const int drawCount = std::min(qLen, maxFit);
                for (int j = 0; j < drawCount; ++j) {
                    const double px = track.left() + 3.0 + j * (kQueuePersonWidth + kQueuePersonGap);
                    painter.drawRoundedRect(QRectF(px, track.top() + 2.0, kQueuePersonWidth, track.height() - 4.0), 2.0, 2.0);
                }
                if (qLen > drawCount) {
                    painter.setPen(QColor("#1E293B"));
                    painter.setFont(labelF);
                    painter.drawText(QRectF(track.right() - 52.0, track.top(), 48.0, track.height()),
                                     Qt::AlignRight | Qt::AlignVCenter,
                                     QString("+%1").arg(qLen - drawCount));
                }
            }

            painter.setPen(QColor("#0F172A"));
            QFont cF = painter.font();
            cF.setBold(true);
            painter.setFont(cF);
            painter.drawText(QRectF(track.right() + 4.0, y, 36.0, rowH),
                             Qt::AlignLeft | Qt::AlignVCenter, QString::number(qLen));
            painter.setFont(labelF);
        }
    }
    painter.restore();

    // 右：餐桌矩阵
    painter.save();
    const auto& canteen = engine_->canteen();
    const int rows = canteen.rows();
    const int cols = canteen.cols();

    painter.setPen(QColor("#0F172A"));
    painter.setFont(titleF);
    painter.drawText(QRectF(right.left(), right.top(), right.width(), 20),
                     Qt::AlignLeft,
                     QString("餐桌矩阵 %1×%2  · 已就座 %3")
                         .arg(rows).arg(cols).arg(canteen.getOccupiedSeats()));

    const double padTop = 24.0;
    const double cw = right.width();
    const double ch = right.height() - padTop;
    if (rows > 0 && cols > 0 && cw > 0 && ch > 0) {
        const double cell = std::min(cw / static_cast<double>(cols), ch / static_cast<double>(rows));
        const double gridW = cell * cols;
        const double gridH = cell * rows;
        const double ox = right.left() + (cw - gridW) / 2.0;
        const double oy = right.top() + padTop + (ch - gridH) / 2.0;

        const QColor emptyColor("#E2E8F0");
        const QColor edgeColor("#94A3B8");

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                const QRectF rc(ox + c * cell + 1.0, oy + r * cell + 1.0, cell - 2.0, cell - 2.0);
                const bool occupied = canteen.isSeatOccupied(r, c);
                painter.setPen(QPen(edgeColor, 0.5));
                if (occupied && engine_->config().enablePreferences) {
                    // 取学生画像颜色 —— 简化处理，按 r+c 循环
                    static const QColor colors[] = {
                        profileColor(core::ProfileType::Undergrad),
                        profileColor(core::ProfileType::Grad),
                        profileColor(core::ProfileType::Staff)};
                    painter.setBrush(colors[(r + c) % 3]);
                } else if (occupied) {
                    painter.setBrush(QColor("#22C55E"));
                } else {
                    painter.setBrush(emptyColor);
                }
                painter.drawRoundedRect(rc, std::min(3.0, cell * 0.15), std::min(3.0, cell * 0.15));
            }
        }

        // 图例
        const double legY = oy + gridH + 6.0;
        if (legY + 16.0 < right.bottom()) {
            QFont legF = painter.font();
            legF.setPointSize(8);
            legF.setBold(false);
            painter.setFont(legF);

            auto drawLegend = [&](double x, const QColor& color, const QString& text) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(color);
                painter.drawRoundedRect(QRectF(x, legY, 14.0, 12.0), 2.0, 2.0);
                painter.setPen(QColor("#475569"));
                painter.drawText(QRectF(x + 18.0, legY - 1, 64.0, 14.0), Qt::AlignLeft | Qt::AlignVCenter, text);
            };

            double lx = right.left();
            if (engine_->config().enablePreferences) {
                drawLegend(lx, profileColor(core::ProfileType::Undergrad), QStringLiteral("本科生"));
                lx += 88.0;
                drawLegend(lx, profileColor(core::ProfileType::Grad), QStringLiteral("研究生"));
                lx += 80.0;
                drawLegend(lx, profileColor(core::ProfileType::Staff), QStringLiteral("教职工"));
            } else {
                drawLegend(lx, QColor("#22C55E"), QStringLiteral("有人"));
                lx += 76.0;
                drawLegend(lx, QColor("#E2E8F0"), QStringLiteral("空闲"));
            }
        }
    }
    painter.restore();
}

} // namespace bdss::gui
