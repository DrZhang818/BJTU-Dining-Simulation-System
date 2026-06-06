#pragma once

#include "core/Config.h"
#include "core/SimulationEngine.h"

#include <QMainWindow>
#include <QTimer>

#include <filesystem>
#include <memory>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSlider;
class QSpinBox;
class QTabWidget;
class QTimeEdit;
class QWidget;

namespace bdss::gui {

class ChartWidget;
class RenderWidget;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(std::filesystem::path defaultConfigPath = {}, QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    QWidget* buildControlBar();
    QWidget* buildConfigTab();
    QWidget* buildLiveTab();
    QWidget* buildChartsTab();
    QWidget* makeMetricCard(const QString& title, QLabel*& valueLabel);

    void connectDirtySignals();
    void markConfigDirty();
    void onLoadDefaults();
    void onSaveConfig();
    void onApplyConfig();
    void onStartClicked();
    void onPauseClicked();
    void onStepClicked();
    void onResetClicked();
    void onExportCsv();
    void onSpeedChanged(int value);
    void onTick();

    void recreateEngine();
    void refreshLiveStats();
    void writeConfigToForm(const bdss::core::Config& config);
    bdss::core::Config readConfigFromForm() const;
    void setRunningUi(bool running);
    void resetMetrics();
    void updateDirtyStatus();

    std::filesystem::path defaultConfigPath_;
    std::unique_ptr<bdss::core::SimulationEngine> engine_;
    QTimer* timer_ = nullptr;
    QTabWidget* tabs_ = nullptr;
    RenderWidget* renderWidget_ = nullptr;
    ChartWidget* chartWidget_ = nullptr;
    bool configDirty_ = true;
    bool suppressDirty_ = false;
    int ticksPerFrame_ = 20;

    QPushButton* btnStart_ = nullptr;
    QPushButton* btnPause_ = nullptr;
    QPushButton* btnStep_ = nullptr;
    QPushButton* btnReset_ = nullptr;
    QPushButton* btnApply_ = nullptr;
    QPushButton* btnExportCsv_ = nullptr;
    QSlider* sliderSpeed_ = nullptr;
    QLabel* labelSpeed_ = nullptr;
    QLabel* labelDirty_ = nullptr;

    QSpinBox* spinWindowCount_ = nullptr;
    QSpinBox* spinTableRows_ = nullptr;
    QSpinBox* spinTableCols_ = nullptr;
    QSpinBox* spinTotalSimTime_ = nullptr;
    QSpinBox* spinRandomSeed_ = nullptr;
    QDoubleSpinBox* spinArrivalRate_ = nullptr;
    QSpinBox* spinAvgServiceTime_ = nullptr;
    QDoubleSpinBox* spinServiceStddev_ = nullptr;
    QSpinBox* spinAvgDiningTime_ = nullptr;
    QDoubleSpinBox* spinDiningStddev_ = nullptr;
    QComboBox* comboArrivalPattern_ = nullptr;
    QComboBox* comboWindowEfficiency_ = nullptr;
    QTimeEdit* timeRush1Start_ = nullptr;
    QTimeEdit* timeRush1End_ = nullptr;
    QDoubleSpinBox* spinRush1Multiplier_ = nullptr;
    QTimeEdit* timeRush2Start_ = nullptr;
    QTimeEdit* timeRush2End_ = nullptr;
    QDoubleSpinBox* spinRush2Multiplier_ = nullptr;
    QCheckBox* checkPreferences_ = nullptr;
    QDoubleSpinBox* spinQueueWeight_ = nullptr;
    QDoubleSpinBox* spinPreferenceBonus_ = nullptr;
    QDoubleSpinBox* spinEfficiencyWeight_ = nullptr;
    QCheckBox* checkPatience_ = nullptr;
    QDoubleSpinBox* spinAvgPatience_ = nullptr;
    QDoubleSpinBox* spinPatienceStddev_ = nullptr;
    QDoubleSpinBox* spinQueueSwitchProbability_ = nullptr;
    QCheckBox* checkTakeaway_ = nullptr;
    QDoubleSpinBox* spinTakeawayRate_ = nullptr;
    QDoubleSpinBox* spinPackingFactor_ = nullptr;
    QCheckBox* checkCleaning_ = nullptr;
    QSpinBox* spinCleaningTime_ = nullptr;
    QCheckBox* checkGroupDining_ = nullptr;
    QDoubleSpinBox* spinGroupProbability_ = nullptr;
    QSpinBox* spinMaxGroupSize_ = nullptr;
    QCheckBox* checkSeatPreference_ = nullptr;
    QDoubleSpinBox* spinNearWindowWeight_ = nullptr;
    QDoubleSpinBox* spinGroupAdjacencyBonus_ = nullptr;
    QDoubleSpinBox* spinStrangerPenalty_ = nullptr;

    QLabel* statTime_ = nullptr;
    QLabel* statQueue_ = nullptr;
    QLabel* statWaitSeat_ = nullptr;
    QLabel* statOccupied_ = nullptr;
    QLabel* statCleaning_ = nullptr;
    QLabel* statFinished_ = nullptr;
    QLabel* statDropped_ = nullptr;
    QLabel* statTakeaway_ = nullptr;
    QLabel* statUtilization_ = nullptr;
    QLabel* statGenerated_ = nullptr;
};

} // namespace bdss::gui
