#pragma once

#include "core/Config.h"
#include "core/SimulationEngine.h"
#include "gui/RenderWidget.h"

#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QTimer>

#include <memory>

namespace bdss::gui {

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void startSimulation();
    void resetSimulation();
    void stepSimulation();

private:
    void refreshView();
    core::Config defaultConfig() const;

    std::unique_ptr<core::SimulationEngine> engine_;
    QTimer* timer_ = nullptr;
    QLabel* summaryLabel_ = nullptr;
    RenderWidget* renderWidget_ = nullptr;
    QPushButton* startButton_ = nullptr;
};

} // namespace bdss::gui
