#pragma once

#include "core/Config.h"
#include "core/SimulationEngine.h"
#include "gui/RenderWidget.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QTimeEdit>
#include <QTimer>
#include <QTabWidget>
#include <QWidget>

#include <memory>

class QChartView;
class QChart;

namespace bdss::gui {

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void startSimulation();
    void resetSimulation();
    void stepSimulation();
    void onTick();

private:
    QWidget* buildControlBar();
    QWidget* buildConfigTab();
    QWidget* buildLiveTab();
    QWidget* buildChartsTab();

    void refreshView();
    void refreshLiveStats();
    core::Config readConfigFromForm() const;
    void writeConfigToForm(const core::Config& config);
    void setRunningUi(bool running);
    void exportCsv();
    void updateCharts();

    std::unique_ptr<core::SimulationEngine> engine_;
    QTimer* timer_ = nullptr;
    QTabWidget* tabs_ = nullptr;

    // 控制栏
    QPushButton* btnStart_ = nullptr;
    QPushButton* btnPause_ = nullptr;
    QPushButton* btnStep_ = nullptr;
    QPushButton* btnReset_ = nullptr;
    QPushButton* btnExportCsv_ = nullptr;

    // 参数配置 Tab
    QSpinBox* spinWindowCount_ = nullptr;
    QSpinBox* spinTableRows_ = nullptr;
    QSpinBox* spinTableCols_ = nullptr;
    QSpinBox* spinTotalSimTime_ = nullptr;
    QDoubleSpinBox* spinArrivalRate_ = nullptr;
    QSpinBox* spinAvgServiceTime_ = nullptr;
    QDoubleSpinBox* spinServiceStddev_ = nullptr;
    QSpinBox* spinAvgDiningTime_ = nullptr;
    QDoubleSpinBox* spinDiningStddev_ = nullptr;
    QTimeEdit* timeRush1Start_ = nullptr;
    QTimeEdit* timeRush1End_ = nullptr;
    QDoubleSpinBox* spinRush1Mult_ = nullptr;
    QTimeEdit* timeRush2Start_ = nullptr;
    QTimeEdit* timeRush2End_ = nullptr;
    QDoubleSpinBox* spinRush2Mult_ = nullptr;
    QSpinBox* spinRandomSeed_ = nullptr;

    // 偏好参数
    QCheckBox* checkEnablePreferences_ = nullptr;
    QDoubleSpinBox* spinRatioUndergrad_ = nullptr;
    QDoubleSpinBox* spinRatioGrad_ = nullptr;
    QDoubleSpinBox* spinRatioStaff_ = nullptr;
    QDoubleSpinBox* spinUndergradDiningFactor_ = nullptr;
    QDoubleSpinBox* spinStaffDiningFactor_ = nullptr;
    QDoubleSpinBox* spinQueueLengthWeight_ = nullptr;
    QDoubleSpinBox* spinPreferenceBonus_ = nullptr;

    // Phase 2 参数
    QCheckBox* checkEnablePatience_ = nullptr;
    QDoubleSpinBox* spinPatienceBase_ = nullptr;
    QDoubleSpinBox* spinPatienceStddev_ = nullptr;
    QCheckBox* checkPatienceAllowSwitch_ = nullptr;
    QCheckBox* checkEnablePacking_ = nullptr;
    QDoubleSpinBox* spinPackingRatio_ = nullptr;
    QDoubleSpinBox* spinPackingServiceFactor_ = nullptr;
    QCheckBox* checkEnableCleaning_ = nullptr;
    QSpinBox* spinCleaningTime_ = nullptr;
    QCheckBox* checkEnableGroupDining_ = nullptr;
    QDoubleSpinBox* spinGroupRatio_ = nullptr;
    QSpinBox* spinGroupSizeMin_ = nullptr;
    QSpinBox* spinGroupSizeMax_ = nullptr;

    // 座位偏好
    QCheckBox* checkEnableSeatPreference_ = nullptr;
    QDoubleSpinBox* spinWindowProximityWeight_ = nullptr;
    QDoubleSpinBox* spinGroupProximityWeight_ = nullptr;
    QDoubleSpinBox* spinIsolationWeight_ = nullptr;

    // 实时监控
    RenderWidget* renderWidget_ = nullptr;
    QLabel* statTime_ = nullptr;
    QLabel* statQueue_ = nullptr;
    QLabel* statWaitSeat_ = nullptr;
    QLabel* statOccupied_ = nullptr;
    QLabel* statUtilization_ = nullptr;
    QLabel* statFinished_ = nullptr;

    // 图表页
    QChartView* chartView_ = nullptr;
    QChart* chart_ = nullptr;
    QLabel* summaryPanel_ = nullptr;
};

} // namespace bdss::gui
