#include "gui/MainWindow.h"

#include "gui/ChartWidget.h"
#include "gui/RenderWidget.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTime>
#include <QTimeEdit>
#include <QVBoxLayout>
#include <QVariant>

#include <algorithm>
#include <array>

namespace bdss::gui {
namespace {

constexpr std::array<int, 6> kSpeedSteps{1, 5, 20, 100, 500, 1000};

QString humanizeSeconds(int totalSeconds) {
    const int h = totalSeconds / 3600;
    const int m = (totalSeconds % 3600) / 60;
    const int s = totalSeconds % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

QTime timeFromSeconds(int seconds) {
    seconds = ((seconds % (24 * 3600)) + 24 * 3600) % (24 * 3600);
    return QTime(seconds / 3600, (seconds % 3600) / 60, seconds % 60);
}

int secondsFromTime(const QTime& time) {
    return time.hour() * 3600 + time.minute() * 60 + time.second();
}

QLabel* makeValueLabel(const QString& text = QStringLiteral("--")) {
    auto* label = new QLabel(text);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QStringLiteral("font-size:20px;font-weight:700;color:#0F172A;"));
    return label;
}

QSpinBox* spin(int minValue, int maxValue, int value, const QString& suffix = {}) {
    auto* s = new QSpinBox;
    s->setRange(minValue, maxValue);
    s->setValue(value);
    s->setSuffix(suffix);
    s->setMinimumWidth(118);
    return s;
}

QDoubleSpinBox* dspin(double minValue, double maxValue, double value, int decimals = 2, const QString& suffix = {}) {
    auto* s = new QDoubleSpinBox;
    s->setRange(minValue, maxValue);
    s->setDecimals(decimals);
    s->setValue(value);
    s->setSuffix(suffix);
    s->setMinimumWidth(118);
    return s;
}

QTimeEdit* timeEdit(int seconds) {
    auto* t = new QTimeEdit(timeFromSeconds(seconds));
    t->setDisplayFormat(QStringLiteral("HH:mm:ss"));
    t->setMinimumWidth(118);
    return t;
}

void styleGroup(QGroupBox* group) {
    group->setStyleSheet(QStringLiteral(
        "QGroupBox{font-weight:700;border:1px solid #E2E8F0;border-radius:14px;margin-top:14px;padding:12px;background:#FFFFFF;}"
        "QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 6px;color:#0F172A;}"));
}

} // namespace

MainWindow::MainWindow(std::filesystem::path defaultConfigPath, QWidget* parent)
    : QMainWindow(parent), defaultConfigPath_(std::move(defaultConfigPath)), timer_(new QTimer(this)) {
    setWindowTitle(QStringLiteral("BDSS · 北京交通大学就餐仿真系统 v3.0"));
    resize(1360, 860);
    setMinimumSize(1100, 720);
    setStyleSheet(QStringLiteral(
        "QMainWindow{background:#F8FAFC;}"
        "QPushButton{border:1px solid #CBD5E1;border-radius:10px;padding:7px 12px;background:#FFFFFF;color:#0F172A;}"
        "QPushButton:hover{background:#F1F5F9;}"
        "QPushButton:disabled{color:#94A3B8;background:#F8FAFC;}"
        "QPushButton#primary{background:#2563EB;color:white;border-color:#2563EB;font-weight:700;}"
        "QPushButton#danger{background:#FEF2F2;color:#B91C1C;border-color:#FECACA;}"
        "QTabWidget::pane{border:0;}"
        "QTabBar::tab{border:1px solid #E2E8F0;border-radius:9px;padding:7px 14px;margin:3px;background:#FFFFFF;}"
        "QTabBar::tab:selected{background:#2563EB;color:white;border-color:#2563EB;}"
        "QSpinBox,QDoubleSpinBox,QComboBox,QTimeEdit{border:1px solid #CBD5E1;border-radius:8px;padding:4px;background:#FFFFFF;}"));

    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);
    root->addWidget(buildControlBar());
    tabs_ = new QTabWidget(central);
    tabs_->addTab(buildConfigTab(), QStringLiteral("① 参数配置"));
    tabs_->addTab(buildLiveTab(), QStringLiteral("② 实时监控"));
    tabs_->addTab(buildChartsTab(), QStringLiteral("③ 数据分析"));
    tabs_->setCurrentIndex(1);
    root->addWidget(tabs_, 1);
    setCentralWidget(central);

    connect(timer_, &QTimer::timeout, this, &MainWindow::onTick);
    timer_->setInterval(40);
    connectDirtySignals();
    onLoadDefaults();
    setRunningUi(false);
    resetMetrics();
    statusBar()->showMessage(QStringLiteral("就绪：修改参数后点击“应用参数”，确认内核已同步后再开始仿真。"));
}

MainWindow::~MainWindow() = default;

QWidget* MainWindow::buildControlBar() {
    auto* bar = new QFrame;
    bar->setStyleSheet(QStringLiteral("QFrame{background:#FFFFFF;border:1px solid #E2E8F0;border-radius:16px;}"));
    auto* layout = new QHBoxLayout(bar);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(10);

    auto* title = new QLabel(QStringLiteral("BDSS 食堂高峰期离散事件仿真"));
    title->setStyleSheet(QStringLiteral("font-size:17px;font-weight:800;color:#0F172A;"));

    btnStart_ = new QPushButton(QStringLiteral("▶ 开始"));
    btnStart_->setObjectName(QStringLiteral("primary"));
    btnPause_ = new QPushButton(QStringLiteral("⏸ 暂停"));
    btnStep_ = new QPushButton(QStringLiteral("⏭ 单步"));
    btnReset_ = new QPushButton(QStringLiteral("↺ 重置"));
    btnReset_->setObjectName(QStringLiteral("danger"));
    btnApply_ = new QPushButton(QStringLiteral("✓ 应用参数"));
    btnExportCsv_ = new QPushButton(QStringLiteral("导出 CSV"));

    connect(btnStart_, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(btnPause_, &QPushButton::clicked, this, &MainWindow::onPauseClicked);
    connect(btnStep_, &QPushButton::clicked, this, &MainWindow::onStepClicked);
    connect(btnReset_, &QPushButton::clicked, this, &MainWindow::onResetClicked);
    connect(btnApply_, &QPushButton::clicked, this, &MainWindow::onApplyConfig);
    connect(btnExportCsv_, &QPushButton::clicked, this, &MainWindow::onExportCsv);

    sliderSpeed_ = new QSlider(Qt::Horizontal);
    sliderSpeed_->setRange(0, static_cast<int>(kSpeedSteps.size()) - 1);
    sliderSpeed_->setValue(2);
    sliderSpeed_->setFixedWidth(170);
    labelSpeed_ = new QLabel;
    labelSpeed_->setMinimumWidth(78);
    connect(sliderSpeed_, &QSlider::valueChanged, this, &MainWindow::onSpeedChanged);
    onSpeedChanged(sliderSpeed_->value());

    labelDirty_ = new QLabel;
    labelDirty_->setMinimumWidth(210);

    layout->addWidget(title);
    layout->addStretch(1);
    layout->addWidget(btnStart_);
    layout->addWidget(btnPause_);
    layout->addWidget(btnStep_);
    layout->addWidget(btnReset_);
    layout->addSpacing(8);
    layout->addWidget(new QLabel(QStringLiteral("速度")));
    layout->addWidget(sliderSpeed_);
    layout->addWidget(labelSpeed_);
    layout->addWidget(btnApply_);
    layout->addWidget(btnExportCsv_);
    layout->addWidget(labelDirty_);
    return bar;
}

QWidget* MainWindow::buildConfigTab() {
    auto* page = new QWidget;
    auto* outer = new QVBoxLayout(page);
    auto* scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* content = new QWidget(scroll);
    auto* grid = new QGridLayout(content);
    grid->setContentsMargins(6, 6, 6, 6);
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(10);

    auto* scale = new QGroupBox(QStringLiteral("仿真规模"));
    styleGroup(scale);
    auto* f1 = new QFormLayout(scale);
    spinWindowCount_ = spin(1, 100, 8);
    spinTableRows_ = spin(1, 80, 10);
    spinTableCols_ = spin(1, 80, 10);
    spinTotalSimTime_ = spin(1, 24 * 3600, 6600, QStringLiteral(" 秒"));
    spinRandomSeed_ = spin(0, 100000000, 42);
    f1->addRow(QStringLiteral("餐口数量"), spinWindowCount_);
    f1->addRow(QStringLiteral("餐桌行数"), spinTableRows_);
    f1->addRow(QStringLiteral("餐桌列数"), spinTableCols_);
    f1->addRow(QStringLiteral("仿真总时长"), spinTotalSimTime_);
    f1->addRow(QStringLiteral("随机种子"), spinRandomSeed_);

    auto* flow = new QGroupBox(QStringLiteral("人流与服务"));
    styleGroup(flow);
    auto* f2 = new QFormLayout(flow);
    spinArrivalRate_ = dspin(0.0, 1000.0, 6.0, 2, QStringLiteral(" 人/分钟"));
    spinAvgServiceTime_ = spin(1, 3600, 25, QStringLiteral(" 秒"));
    spinServiceStddev_ = dspin(0.0, 1800.0, 8.0, 1, QStringLiteral(" 秒"));
    spinAvgDiningTime_ = spin(1, 7200, 900, QStringLiteral(" 秒"));
    spinDiningStddev_ = dspin(0.0, 3600.0, 240.0, 1, QStringLiteral(" 秒"));
    f2->addRow(QStringLiteral("平均到达率"), spinArrivalRate_);
    f2->addRow(QStringLiteral("平均打饭耗时"), spinAvgServiceTime_);
    f2->addRow(QStringLiteral("打饭耗时标准差"), spinServiceStddev_);
    f2->addRow(QStringLiteral("平均就餐时长"), spinAvgDiningTime_);
    f2->addRow(QStringLiteral("就餐时长标准差"), spinDiningStddev_);

    auto* rush = new QGroupBox(QStringLiteral("高峰与窗口选择"));
    styleGroup(rush);
    auto* f3 = new QFormLayout(rush);
    comboArrivalPattern_ = new QComboBox;
    comboArrivalPattern_->addItem(QStringLiteral("稳定到达"), static_cast<int>(bdss::core::ArrivalPattern::Steady));
    comboArrivalPattern_->addItem(QStringLiteral("多段高峰"), static_cast<int>(bdss::core::ArrivalPattern::RushPeaks));
    comboWindowEfficiency_ = new QComboBox;
    comboWindowEfficiency_->addItem(QStringLiteral("效率一致"), static_cast<int>(bdss::core::WindowEfficiency::Uniform));
    comboWindowEfficiency_->addItem(QStringLiteral("快慢窗口混合"), static_cast<int>(bdss::core::WindowEfficiency::Variable));
    comboWindowEfficiency_->addItem(QStringLiteral("自定义配置文件"), static_cast<int>(bdss::core::WindowEfficiency::Custom));
    timeRush1Start_ = timeEdit(900);
    timeRush1End_ = timeEdit(2400);
    spinRush1Multiplier_ = dspin(0.0, 20.0, 2.5, 2);
    timeRush2Start_ = timeEdit(3000);
    timeRush2End_ = timeEdit(4500);
    spinRush2Multiplier_ = dspin(0.0, 20.0, 1.8, 2);
    spinQueueWeight_ = dspin(0.0, 20.0, 1.0, 2);
    spinPreferenceBonus_ = dspin(0.0, 50.0, 3.0, 2);
    spinEfficiencyWeight_ = dspin(0.0, 20.0, 1.0, 2);
    f3->addRow(QStringLiteral("到达模式"), comboArrivalPattern_);
    f3->addRow(QStringLiteral("窗口效率"), comboWindowEfficiency_);
    f3->addRow(QStringLiteral("第一段开始"), timeRush1Start_);
    f3->addRow(QStringLiteral("第一段结束"), timeRush1End_);
    f3->addRow(QStringLiteral("第一段倍率"), spinRush1Multiplier_);
    f3->addRow(QStringLiteral("第二段开始"), timeRush2Start_);
    f3->addRow(QStringLiteral("第二段结束"), timeRush2End_);
    f3->addRow(QStringLiteral("第二段倍率"), spinRush2Multiplier_);
    f3->addRow(QStringLiteral("队列权重"), spinQueueWeight_);
    f3->addRow(QStringLiteral("偏好加分"), spinPreferenceBonus_);
    f3->addRow(QStringLiteral("效率权重"), spinEfficiencyWeight_);

    auto* behavior = new QGroupBox(QStringLiteral("行为模型"));
    styleGroup(behavior);
    auto* f4 = new QFormLayout(behavior);
    checkPreferences_ = new QCheckBox(QStringLiteral("启用餐口偏好"));
    checkPatience_ = new QCheckBox(QStringLiteral("启用耐心/离队"));
    spinAvgPatience_ = dspin(1.0, 7200.0, 420.0, 1, QStringLiteral(" 秒"));
    spinPatienceStddev_ = dspin(0.0, 3600.0, 90.0, 1, QStringLiteral(" 秒"));
    spinQueueSwitchProbability_ = dspin(0.0, 1.0, 0.12, 2);
    checkTakeaway_ = new QCheckBox(QStringLiteral("启用外带打包"));
    spinTakeawayRate_ = dspin(0.0, 1.0, 0.18, 2);
    spinPackingFactor_ = dspin(1.0, 5.0, 1.25, 2);
    checkCleaning_ = new QCheckBox(QStringLiteral("启用座位清洁"));
    spinCleaningTime_ = spin(0, 3600, 45, QStringLiteral(" 秒"));
    checkGroupDining_ = new QCheckBox(QStringLiteral("启用结伴用餐"));
    spinGroupProbability_ = dspin(0.0, 1.0, 0.22, 2);
    spinMaxGroupSize_ = spin(1, 12, 4);
    checkSeatPreference_ = new QCheckBox(QStringLiteral("启用座位偏好"));
    spinNearWindowWeight_ = dspin(0.0, 10.0, 0.25, 2);
    spinGroupAdjacencyBonus_ = dspin(0.0, 20.0, 2.5, 2);
    spinStrangerPenalty_ = dspin(0.0, 20.0, 0.5, 2);
    f4->addRow(checkPreferences_);
    f4->addRow(checkPatience_);
    f4->addRow(QStringLiteral("平均耐心"), spinAvgPatience_);
    f4->addRow(QStringLiteral("耐心标准差"), spinPatienceStddev_);
    f4->addRow(QStringLiteral("换队概率"), spinQueueSwitchProbability_);
    f4->addRow(checkTakeaway_);
    f4->addRow(QStringLiteral("外带比例"), spinTakeawayRate_);
    f4->addRow(QStringLiteral("打包耗时倍率"), spinPackingFactor_);
    f4->addRow(checkCleaning_);
    f4->addRow(QStringLiteral("清洁耗时"), spinCleaningTime_);
    f4->addRow(checkGroupDining_);
    f4->addRow(QStringLiteral("结伴概率"), spinGroupProbability_);
    f4->addRow(QStringLiteral("最大同行人数"), spinMaxGroupSize_);
    f4->addRow(checkSeatPreference_);
    f4->addRow(QStringLiteral("靠近窗口权重"), spinNearWindowWeight_);
    f4->addRow(QStringLiteral("同行邻座奖励"), spinGroupAdjacencyBonus_);
    f4->addRow(QStringLiteral("陌生人间隔惩罚"), spinStrangerPenalty_);

    auto* buttons = new QWidget;
    auto* buttonLayout = new QHBoxLayout(buttons);
    auto* load = new QPushButton(QStringLiteral("恢复默认配置"));
    auto* save = new QPushButton(QStringLiteral("保存当前配置"));
    connect(load, &QPushButton::clicked, this, &MainWindow::onLoadDefaults);
    connect(save, &QPushButton::clicked, this, &MainWindow::onSaveConfig);
    buttonLayout->addStretch();
    buttonLayout->addWidget(load);
    buttonLayout->addWidget(save);

    grid->addWidget(scale, 0, 0);
    grid->addWidget(flow, 0, 1);
    grid->addWidget(rush, 0, 2);
    grid->addWidget(behavior, 0, 3);
    grid->addWidget(buttons, 1, 0, 1, 4);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);
    grid->setColumnStretch(3, 1);
    scroll->setWidget(content);
    outer->addWidget(scroll);
    return page;
}

QWidget* MainWindow::buildLiveTab() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    renderWidget_ = new RenderWidget(page);
    layout->addWidget(renderWidget_, 1);
    auto* metrics = new QFrame;
    metrics->setStyleSheet(QStringLiteral("QFrame{background:#FFFFFF;border:1px solid #E2E8F0;border-radius:16px;}"));
    auto* grid = new QGridLayout(metrics);
    grid->setContentsMargins(10, 10, 10, 10);
    grid->setSpacing(8);
    grid->addWidget(makeMetricCard(QStringLiteral("仿真时间"), statTime_), 0, 0);
    grid->addWidget(makeMetricCard(QStringLiteral("已生成"), statGenerated_), 0, 1);
    grid->addWidget(makeMetricCard(QStringLiteral("窗口排队"), statQueue_), 0, 2);
    grid->addWidget(makeMetricCard(QStringLiteral("等座人数"), statWaitSeat_), 0, 3);
    grid->addWidget(makeMetricCard(QStringLiteral("已就座"), statOccupied_), 0, 4);
    grid->addWidget(makeMetricCard(QStringLiteral("清洁中"), statCleaning_), 0, 5);
    grid->addWidget(makeMetricCard(QStringLiteral("已完成"), statFinished_), 0, 6);
    grid->addWidget(makeMetricCard(QStringLiteral("已离队"), statDropped_), 0, 7);
    grid->addWidget(makeMetricCard(QStringLiteral("外带"), statTakeaway_), 0, 8);
    grid->addWidget(makeMetricCard(QStringLiteral("座位利用率"), statUtilization_), 0, 9);
    layout->addWidget(metrics);
    return page;
}

QWidget* MainWindow::buildChartsTab() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    chartWidget_ = new ChartWidget(page);
    layout->addWidget(chartWidget_, 1);
    return page;
}

QWidget* MainWindow::makeMetricCard(const QString& title, QLabel*& valueLabel) {
    auto* card = new QWidget;
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(8, 6, 8, 6);
    auto* titleLabel = new QLabel(title);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QStringLiteral("color:#64748B;font-size:12px;"));
    valueLabel = makeValueLabel();
    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);
    return card;
}

void MainWindow::connectDirtySignals() {
    auto connectSpin = [&](QSpinBox* w) { connect(w, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) { markConfigDirty(); }); };
    auto connectDouble = [&](QDoubleSpinBox* w) { connect(w, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double) { markConfigDirty(); }); };
    auto connectCheck = [&](QCheckBox* w) { connect(w, &QCheckBox::toggled, this, [this](bool) { markConfigDirty(); }); };
    auto connectCombo = [&](QComboBox* w) { connect(w, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) { markConfigDirty(); }); };
    auto connectTime = [&](QTimeEdit* w) { connect(w, &QTimeEdit::timeChanged, this, [this](const QTime&) { markConfigDirty(); }); };

    for (auto* w : {spinWindowCount_, spinTableRows_, spinTableCols_, spinTotalSimTime_, spinRandomSeed_, spinAvgServiceTime_, spinAvgDiningTime_, spinCleaningTime_, spinMaxGroupSize_}) connectSpin(w);
    for (auto* w : {spinArrivalRate_, spinServiceStddev_, spinDiningStddev_, spinRush1Multiplier_, spinRush2Multiplier_, spinQueueWeight_, spinPreferenceBonus_, spinEfficiencyWeight_, spinAvgPatience_, spinPatienceStddev_, spinQueueSwitchProbability_, spinTakeawayRate_, spinPackingFactor_, spinGroupProbability_, spinNearWindowWeight_, spinGroupAdjacencyBonus_, spinStrangerPenalty_}) connectDouble(w);
    for (auto* w : {checkPreferences_, checkPatience_, checkTakeaway_, checkCleaning_, checkGroupDining_, checkSeatPreference_}) connectCheck(w);
    for (auto* w : {comboArrivalPattern_, comboWindowEfficiency_}) connectCombo(w);
    for (auto* w : {timeRush1Start_, timeRush1End_, timeRush2Start_, timeRush2End_}) connectTime(w);
}

void MainWindow::markConfigDirty() {
    if (suppressDirty_) return;
    configDirty_ = true;
    updateDirtyStatus();
    statusBar()->showMessage(QStringLiteral("参数已修改：点击“应用参数”会同步到仿真内核；新增餐口会立即参与路由。"), 3000);
}

void MainWindow::onLoadDefaults() {
    bdss::core::Config config;
    try {
        if (!defaultConfigPath_.empty() && std::filesystem::exists(defaultConfigPath_)) {
            config = bdss::core::Config::loadFromFile(defaultConfigPath_);
        }
        writeConfigToForm(config);
        configDirty_ = true;
        updateDirtyStatus();
        statusBar()->showMessage(QStringLiteral("已载入默认配置。"), 2000);
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, QStringLiteral("配置错误"), QString::fromLocal8Bit(ex.what()));
    }
}

void MainWindow::onSaveConfig() {
    const QString file = QFileDialog::getSaveFileName(this, QStringLiteral("保存配置"), QStringLiteral("bdss_config.json"), QStringLiteral("JSON (*.json)"));
    if (file.isEmpty()) return;
    try {
        readConfigFromForm().saveToFile(file.toStdString());
        statusBar()->showMessage(QStringLiteral("配置已保存：%1").arg(file), 2500);
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QString::fromLocal8Bit(ex.what()));
    }
}

void MainWindow::onApplyConfig() {
    try {
        auto config = readConfigFromForm();
        config.normalize();
        config.validate();
        if (engine_) {
            engine_->updateConfig(config, true);
        } else {
            engine_ = std::make_unique<bdss::core::SimulationEngine>(config);
            if (renderWidget_) renderWidget_->setEngine(engine_.get());
            if (chartWidget_) chartWidget_->clear();
            resetMetrics();
        }
        configDirty_ = false;
        if (renderWidget_) renderWidget_->setEngine(engine_.get());
        refreshLiveStats();
        updateDirtyStatus();
        tabs_->setCurrentIndex(1);
        statusBar()->showMessage(QStringLiteral("参数已同步到仿真内核，新增餐口已参与排队路由。"), 3000);
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, QStringLiteral("参数无效"), QString::fromLocal8Bit(ex.what()));
    }
}

void MainWindow::onStartClicked() {
    try {
        if (!engine_ || engine_->isFinished()) {
            recreateEngine();
        } else if (configDirty_) {
            onApplyConfig();
        }
        timer_->start();
        setRunningUi(true);
        tabs_->setCurrentIndex(1);
        statusBar()->showMessage(QStringLiteral("仿真运行中…"));
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, QStringLiteral("无法开始"), QString::fromLocal8Bit(ex.what()));
    }
}

void MainWindow::onPauseClicked() {
    timer_->stop();
    setRunningUi(false);
    statusBar()->showMessage(QStringLiteral("已暂停。修改参数后点击“应用参数”可实时同步，不需要重置。"), 3000);
}

void MainWindow::onStepClicked() {
    try {
        if (!engine_ || engine_->isFinished()) {
            recreateEngine();
        } else if (configDirty_) {
            onApplyConfig();
        }
        if (!engine_->isFinished()) engine_->tick();
        refreshLiveStats();
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, QStringLiteral("单步失败"), QString::fromLocal8Bit(ex.what()));
    }
}

void MainWindow::onResetClicked() {
    timer_->stop();
    engine_.reset();
    if (renderWidget_) renderWidget_->setEngine(nullptr);
    if (chartWidget_) chartWidget_->clear();
    resetMetrics();
    configDirty_ = true;
    setRunningUi(false);
    updateDirtyStatus();
    statusBar()->showMessage(QStringLiteral("已重置。"), 2000);
}

void MainWindow::onExportCsv() {
    if (!engine_) {
        QMessageBox::information(this, QStringLiteral("暂无数据"), QStringLiteral("请先运行仿真。"));
        return;
    }
    const QString file = QFileDialog::getSaveFileName(this, QStringLiteral("导出 CSV"), QStringLiteral("simulation_log.csv"), QStringLiteral("CSV (*.csv)"));
    if (file.isEmpty()) return;
    try {
        engine_->getStatistics().exportCsv(file.toStdString());
        statusBar()->showMessage(QStringLiteral("CSV 已导出：%1").arg(file), 3000);
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, QStringLiteral("导出失败"), QString::fromLocal8Bit(ex.what()));
    }
}

void MainWindow::onSpeedChanged(int value) {
    const int idx = std::clamp(value, 0, static_cast<int>(kSpeedSteps.size()) - 1);
    ticksPerFrame_ = kSpeedSteps[static_cast<std::size_t>(idx)];
    if (labelSpeed_) labelSpeed_->setText(QStringLiteral("%1×").arg(ticksPerFrame_));
}

void MainWindow::onTick() {
    if (!engine_) {
        timer_->stop();
        setRunningUi(false);
        return;
    }
    for (int i = 0; i < ticksPerFrame_ && !engine_->isFinished(); ++i) {
        engine_->tick();
    }
    refreshLiveStats();
    if (engine_->isFinished()) {
        timer_->stop();
        setRunningUi(false);
        const auto summary = engine_->getStatistics().getSummary();
        statusBar()->showMessage(QStringLiteral("仿真结束：完成 %1 人，离队 %2 人，平均座位利用率 %3%。")
                                     .arg(summary.finishedStudents)
                                     .arg(summary.droppedStudents)
                                     .arg(summary.averageSeatUtilization * 100.0, 0, 'f', 1));
    }
}

void MainWindow::recreateEngine() {
    auto config = readConfigFromForm();
    config.normalize();
    config.validate();
    engine_ = std::make_unique<bdss::core::SimulationEngine>(config);
    configDirty_ = false;
    if (renderWidget_) renderWidget_->setEngine(engine_.get());
    if (chartWidget_) chartWidget_->clear();
    resetMetrics();
    refreshLiveStats();
    updateDirtyStatus();
    btnExportCsv_->setEnabled(false);
}

void MainWindow::refreshLiveStats() {
    if (!engine_) return;
    statTime_->setText(QStringLiteral("%1 / %2").arg(humanizeSeconds(engine_->getCurrentTime())).arg(humanizeSeconds(engine_->getTotalSimulationTime())));
    statGenerated_->setText(QString::number(engine_->getGeneratedCount()));
    statQueue_->setText(QString::number(engine_->getTotalQueueLength()));
    statWaitSeat_->setText(QString::number(engine_->getWaitingForSeatCount()));
    statOccupied_->setText(QStringLiteral("%1/%2").arg(engine_->getOccupiedSeats()).arg(engine_->getTotalSeats()));
    statCleaning_->setText(QString::number(engine_->getCleaningSeats()));
    statFinished_->setText(QString::number(engine_->getStatistics().getFinishedCount()));
    statDropped_->setText(QString::number(engine_->getDroppedCount()));
    statTakeaway_->setText(QString::number(engine_->getTakeawayCount()));
    statUtilization_->setText(QStringLiteral("%1%").arg(engine_->getSeatUtilization() * 100.0, 0, 'f', 1));
    if (renderWidget_) renderWidget_->update();
    if (chartWidget_) chartWidget_->setRecords(engine_->getStatistics().getTickRecords());
    btnExportCsv_->setEnabled(!engine_->getStatistics().getTickRecords().empty());
}

void MainWindow::writeConfigToForm(const bdss::core::Config& config) {
    suppressDirty_ = true;
    spinWindowCount_->setValue(config.windowCount);
    spinTableRows_->setValue(config.tableRows);
    spinTableCols_->setValue(config.tableCols);
    spinTotalSimTime_->setValue(config.totalSimulationTime);
    spinRandomSeed_->setValue(static_cast<int>(config.randomSeed));
    spinArrivalRate_->setValue(config.arrivalRate);
    spinAvgServiceTime_->setValue(config.avgServiceTime);
    spinServiceStddev_->setValue(config.serviceStddev);
    spinAvgDiningTime_->setValue(config.avgDiningTime);
    spinDiningStddev_->setValue(config.diningStddev);
    comboArrivalPattern_->setCurrentIndex(config.arrivalPattern == bdss::core::ArrivalPattern::RushPeaks ? 1 : 0);
    comboWindowEfficiency_->setCurrentIndex(config.windowEfficiency == bdss::core::WindowEfficiency::Variable ? 1 : (config.windowEfficiency == bdss::core::WindowEfficiency::Custom ? 2 : 0));
    const auto peak1 = config.rushPeaks.size() > 0 ? config.rushPeaks[0] : bdss::core::RushPeak{};
    const auto peak2 = config.rushPeaks.size() > 1 ? config.rushPeaks[1] : bdss::core::RushPeak{3000, 4500, 1.8};
    timeRush1Start_->setTime(timeFromSeconds(peak1.start));
    timeRush1End_->setTime(timeFromSeconds(peak1.end));
    spinRush1Multiplier_->setValue(peak1.multiplier);
    timeRush2Start_->setTime(timeFromSeconds(peak2.start));
    timeRush2End_->setTime(timeFromSeconds(peak2.end));
    spinRush2Multiplier_->setValue(peak2.multiplier);
    checkPreferences_->setChecked(config.enableWindowPreferences);
    spinQueueWeight_->setValue(config.queueWeight);
    spinPreferenceBonus_->setValue(config.preferenceBonus);
    spinEfficiencyWeight_->setValue(config.efficiencyWeight);
    checkPatience_->setChecked(config.enablePatience);
    spinAvgPatience_->setValue(config.avgPatienceTime);
    spinPatienceStddev_->setValue(config.patienceStddev);
    spinQueueSwitchProbability_->setValue(config.queueSwitchProbability);
    checkTakeaway_->setChecked(config.enableTakeaway);
    spinTakeawayRate_->setValue(config.takeawayRate);
    spinPackingFactor_->setValue(config.packingServiceFactor);
    checkCleaning_->setChecked(config.enableCleaning);
    spinCleaningTime_->setValue(config.cleaningTime);
    checkGroupDining_->setChecked(config.enableGroupDining);
    spinGroupProbability_->setValue(config.groupProbability);
    spinMaxGroupSize_->setValue(config.maxGroupSize);
    checkSeatPreference_->setChecked(config.enableSeatPreference);
    spinNearWindowWeight_->setValue(config.nearWindowWeight);
    spinGroupAdjacencyBonus_->setValue(config.groupAdjacencyBonus);
    spinStrangerPenalty_->setValue(config.strangerSpacingPenalty);
    suppressDirty_ = false;
}

bdss::core::Config MainWindow::readConfigFromForm() const {
    bdss::core::Config config;
    config.windowCount = spinWindowCount_->value();
    config.tableRows = spinTableRows_->value();
    config.tableCols = spinTableCols_->value();
    config.totalSimulationTime = spinTotalSimTime_->value();
    config.randomSeed = static_cast<unsigned int>(spinRandomSeed_->value());
    config.arrivalRate = spinArrivalRate_->value();
    config.avgServiceTime = spinAvgServiceTime_->value();
    config.serviceStddev = spinServiceStddev_->value();
    config.avgDiningTime = spinAvgDiningTime_->value();
    config.diningStddev = spinDiningStddev_->value();
    config.arrivalPattern = static_cast<bdss::core::ArrivalPattern>(comboArrivalPattern_->currentData().toInt());
    config.windowEfficiency = static_cast<bdss::core::WindowEfficiency>(comboWindowEfficiency_->currentData().toInt());
    config.rushPeaks = {
        {secondsFromTime(timeRush1Start_->time()), secondsFromTime(timeRush1End_->time()), spinRush1Multiplier_->value()},
        {secondsFromTime(timeRush2Start_->time()), secondsFromTime(timeRush2End_->time()), spinRush2Multiplier_->value()}
    };
    config.enableWindowPreferences = checkPreferences_->isChecked();
    config.queueWeight = spinQueueWeight_->value();
    config.preferenceBonus = spinPreferenceBonus_->value();
    config.efficiencyWeight = spinEfficiencyWeight_->value();
    config.enablePatience = checkPatience_->isChecked();
    config.avgPatienceTime = spinAvgPatience_->value();
    config.patienceStddev = spinPatienceStddev_->value();
    config.queueSwitchProbability = spinQueueSwitchProbability_->value();
    config.enableTakeaway = checkTakeaway_->isChecked();
    config.takeawayRate = spinTakeawayRate_->value();
    config.packingServiceFactor = spinPackingFactor_->value();
    config.enableCleaning = checkCleaning_->isChecked();
    config.cleaningTime = spinCleaningTime_->value();
    config.enableGroupDining = checkGroupDining_->isChecked();
    config.groupProbability = spinGroupProbability_->value();
    config.maxGroupSize = spinMaxGroupSize_->value();
    config.enableSeatPreference = checkSeatPreference_->isChecked();
    config.nearWindowWeight = spinNearWindowWeight_->value();
    config.groupAdjacencyBonus = spinGroupAdjacencyBonus_->value();
    config.strangerSpacingPenalty = spinStrangerPenalty_->value();
    return config;
}

void MainWindow::setRunningUi(bool running) {
    btnStart_->setEnabled(!running);
    btnPause_->setEnabled(running);
    btnStep_->setEnabled(!running);
    btnReset_->setEnabled(true);
    btnApply_->setEnabled(!running && configDirty_);

    const std::vector<QWidget*> controls{
        spinWindowCount_, spinTableRows_, spinTableCols_, spinTotalSimTime_, spinRandomSeed_, spinArrivalRate_, spinAvgServiceTime_, spinServiceStddev_, spinAvgDiningTime_, spinDiningStddev_, comboArrivalPattern_, comboWindowEfficiency_, timeRush1Start_, timeRush1End_, spinRush1Multiplier_, timeRush2Start_, timeRush2End_, spinRush2Multiplier_, checkPreferences_, spinQueueWeight_, spinPreferenceBonus_, spinEfficiencyWeight_, checkPatience_, spinAvgPatience_, spinPatienceStddev_, spinQueueSwitchProbability_, checkTakeaway_, spinTakeawayRate_, spinPackingFactor_, checkCleaning_, spinCleaningTime_, checkGroupDining_, spinGroupProbability_, spinMaxGroupSize_, checkSeatPreference_, spinNearWindowWeight_, spinGroupAdjacencyBonus_, spinStrangerPenalty_};
    for (auto* control : controls) {
        if (control) control->setEnabled(!running);
    }
}

void MainWindow::resetMetrics() {
    for (auto* label : {statTime_, statQueue_, statWaitSeat_, statOccupied_, statCleaning_, statFinished_, statDropped_, statTakeaway_, statUtilization_, statGenerated_}) {
        if (label) label->setText(QStringLiteral("--"));
    }
    if (btnExportCsv_) btnExportCsv_->setEnabled(false);
}

void MainWindow::updateDirtyStatus() {
    if (!labelDirty_) return;
    if (configDirty_) {
        labelDirty_->setText(QStringLiteral("● 参数待应用"));
        labelDirty_->setStyleSheet(QStringLiteral("color:#D97706;font-weight:700;"));
    } else {
        labelDirty_->setText(QStringLiteral("● 内核已同步"));
        labelDirty_->setStyleSheet(QStringLiteral("color:#059669;font-weight:700;"));
    }
    if (btnApply_) btnApply_->setEnabled(!timer_->isActive() && configDirty_);
}

} // namespace bdss::gui
