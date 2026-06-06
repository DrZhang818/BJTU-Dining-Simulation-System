#pragma once

#include "utils/StatisticsLogger.h"

#include <QWidget>
#include <vector>

namespace bdss::gui {

class ChartWidget : public QWidget {
public:
    explicit ChartWidget(QWidget* parent = nullptr);
    void setRecords(const std::vector<bdss::utils::TickRecord>& records);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::vector<bdss::utils::TickRecord> records_;
};

} // namespace bdss::gui
