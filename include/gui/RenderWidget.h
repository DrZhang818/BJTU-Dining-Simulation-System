#pragma once

#include <QWidget>

namespace bdss::gui {

class RenderWidget final : public QWidget {
    Q_OBJECT
public:
    explicit RenderWidget(QWidget* parent = nullptr);
    void setSeatMatrix(int rows, int cols, int occupiedSeats);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int rows_ = 10;
    int cols_ = 10;
    int occupiedSeats_ = 0;
};

} // namespace bdss::gui
