#pragma once

#include "core/Config.h"

#include <QMainWindow>

#include <memory>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSlider;
class QSpinBox;
class QTabWidget;
class QTimer;

namespace bdss::core {
class SimulationEngine;
}

namespace bdss::gui {

class RenderWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onStartClicked();
    void onPauseClicked();
    void onResetClicked();
    void onStepClicked();
    void onSpeedChanged(int value);
    void onTick();
    void onLoadDefaults();

private:
    void buildUi();
    QWidget* buildConfigTab();
    QWidget* buildLiveTab();
    QWidget* buildChartsTab();
    QWidget* buildControlBar();

    bdss::core::Config readConfigFromForm() const;
    void writeConfigToForm(const bdss::core::Config& config);

    void recreateEngine();
    void refreshLiveStats();
    void setRunningUi(bool running);

private:
    QTabWidget* tabs_ = nullptr;

    QSpinBox* spinWindowCount_ = nullptr;
    QSpinBox* spinTableRows_ = nullptr;
    QSpinBox* spinTableCols_ = nullptr;
    QSpinBox* spinTotalSimTime_ = nullptr;
    QDoubleSpinBox* spinArrivalRate_ = nullptr;
    QSpinBox* spinAvgServiceTime_ = nullptr;
    QSpinBox* spinAvgDiningTime_ = nullptr;
    QDoubleSpinBox* spinServiceStddev_ = nullptr;
    QDoubleSpinBox* spinDiningStddev_ = nullptr;
    QComboBox* comboArrivalPattern_ = nullptr;
    QComboBox* comboWindowEfficiency_ = nullptr;
    QSpinBox* spinRushStart_ = nullptr;
    QSpinBox* spinRushEnd_ = nullptr;
    QDoubleSpinBox* spinRushMultiplier_ = nullptr;
    QSpinBox* spinRandomSeed_ = nullptr;

    QPushButton* btnStart_ = nullptr;
    QPushButton* btnPause_ = nullptr;
    QPushButton* btnReset_ = nullptr;
    QPushButton* btnStep_ = nullptr;
    QSlider* sliderSpeed_ = nullptr;
    QLabel* labelSpeed_ = nullptr;

    RenderWidget* renderWidget_ = nullptr;

    QLabel* statTime_ = nullptr;
    QLabel* statQueue_ = nullptr;
    QLabel* statWaitSeat_ = nullptr;
    QLabel* statOccupied_ = nullptr;
    QLabel* statUtilization_ = nullptr;
    QLabel* statFinished_ = nullptr;

    QTimer* timer_ = nullptr;
    int ticksPerFrame_ = 1;

    std::unique_ptr<bdss::core::SimulationEngine> engine_;
};

} // namespace bdss::gui
