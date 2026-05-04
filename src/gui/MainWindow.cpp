#include "gui/MainWindow.h"

#include "core/SimulationEngine.h"
#include "gui/RenderWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <filesystem>

namespace bdss::gui {

namespace {

// 速度档位：每帧 tick 数，对应 1×、5×、20×、100×、500×
constexpr std::array<int, 5> kSpeedSteps{1, 5, 20, 100, 500};

QLabel* makeStatValue(const QString& initial = QStringLiteral("--")) {
    auto* label = new QLabel(initial);
    label->setStyleSheet("font-weight: bold; color: #1976D2; font-size: 16px;");
    label->setAlignment(Qt::AlignCenter);
    return label;
}

QString humanizeSeconds(int totalSeconds) {
    const int h = totalSeconds / 3600;
    const int m = (totalSeconds % 3600) / 60;
    const int s = totalSeconds % 60;
    return QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      timer_(new QTimer(this)) {
    buildUi();

    connect(timer_, &QTimer::timeout, this, &MainWindow::onTick);
    timer_->setInterval(50); // 20 FPS

    onLoadDefaults();
    setRunningUi(false);

    statusBar()->showMessage(QStringLiteral("就绪。请在【参数配置】页设置仿真参数后点击【开始】"));
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi() {
    setWindowTitle(QStringLiteral("BDSS · 北京交通大学就餐仿真系统"));
    resize(1280, 820);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(8, 8, 8, 8);

    layout->addWidget(buildControlBar());

    tabs_ = new QTabWidget(central);
    tabs_->addTab(buildConfigTab(), QStringLiteral("① 参数配置"));
    tabs_->addTab(buildLiveTab(), QStringLiteral("② 实时监控"));
    tabs_->addTab(buildChartsTab(), QStringLiteral("③ 统计图表"));
    tabs_->setCurrentIndex(1); // 默认进入实时监控页

    layout->addWidget(tabs_, /*stretch=*/1);
    setCentralWidget(central);
}

QWidget* MainWindow::buildControlBar() {
    auto* bar = new QWidget;
    auto* layout = new QHBoxLayout(bar);
    layout->setContentsMargins(4, 4, 4, 4);

    btnStart_ = new QPushButton(QStringLiteral("▶ 开始"));
    btnPause_ = new QPushButton(QStringLiteral("⏸ 暂停"));
    btnStep_  = new QPushButton(QStringLiteral("⏭ 单步"));
    btnReset_ = new QPushButton(QStringLiteral("↺ 重置"));

    for (auto* b : {btnStart_, btnPause_, btnStep_, btnReset_}) {
        b->setMinimumHeight(32);
        b->setMinimumWidth(96);
    }

    connect(btnStart_, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(btnPause_, &QPushButton::clicked, this, &MainWindow::onPauseClicked);
    connect(btnStep_,  &QPushButton::clicked, this, &MainWindow::onStepClicked);
    connect(btnReset_, &QPushButton::clicked, this, &MainWindow::onResetClicked);

    sliderSpeed_ = new QSlider(Qt::Horizontal);
    sliderSpeed_->setRange(0, static_cast<int>(kSpeedSteps.size()) - 1);
    sliderSpeed_->setValue(2); // 默认 20×
    sliderSpeed_->setFixedWidth(180);
    connect(sliderSpeed_, &QSlider::valueChanged, this, &MainWindow::onSpeedChanged);

    labelSpeed_ = new QLabel(QStringLiteral("速度: 20×"));
    labelSpeed_->setMinimumWidth(80);
    onSpeedChanged(sliderSpeed_->value());

    layout->addWidget(btnStart_);
    layout->addWidget(btnPause_);
    layout->addWidget(btnStep_);
    layout->addWidget(btnReset_);
    layout->addSpacing(20);
    layout->addWidget(new QLabel(QStringLiteral("仿真速度:")));
    layout->addWidget(sliderSpeed_);
    layout->addWidget(labelSpeed_);
    layout->addStretch();

    return bar;
}

QWidget* MainWindow::buildConfigTab() {
    auto* page = new QWidget;
    auto* root = new QHBoxLayout(page);

    // 左列：仿真规模
    auto* groupScale = new QGroupBox(QStringLiteral("仿真规模"));
    auto* formScale = new QFormLayout(groupScale);

    spinWindowCount_ = new QSpinBox;
    spinWindowCount_->setRange(1, 50);
    formScale->addRow(QStringLiteral("窗口数量"), spinWindowCount_);

    spinTableRows_ = new QSpinBox;
    spinTableRows_->setRange(1, 50);
    formScale->addRow(QStringLiteral("餐桌行数"), spinTableRows_);

    spinTableCols_ = new QSpinBox;
    spinTableCols_->setRange(1, 50);
    formScale->addRow(QStringLiteral("餐桌列数"), spinTableCols_);

    spinTotalSimTime_ = new QSpinBox;
    spinTotalSimTime_->setRange(60, 24 * 3600);
    spinTotalSimTime_->setSuffix(QStringLiteral(" 秒"));
    formScale->addRow(QStringLiteral("仿真总时长"), spinTotalSimTime_);

    spinRandomSeed_ = new QSpinBox;
    spinRandomSeed_->setRange(0, 1'000'000);
    formScale->addRow(QStringLiteral("随机种子"), spinRandomSeed_);

    // 中列：到达与服务
    auto* groupFlow = new QGroupBox(QStringLiteral("人流与服务"));
    auto* formFlow = new QFormLayout(groupFlow);

    spinArrivalRate_ = new QDoubleSpinBox;
    spinArrivalRate_->setRange(0.1, 200.0);
    spinArrivalRate_->setDecimals(2);
    spinArrivalRate_->setSuffix(QStringLiteral(" 人/分钟"));
    formFlow->addRow(QStringLiteral("平均到达率"), spinArrivalRate_);

    spinAvgServiceTime_ = new QSpinBox;
    spinAvgServiceTime_->setRange(1, 600);
    spinAvgServiceTime_->setSuffix(QStringLiteral(" 秒"));
    formFlow->addRow(QStringLiteral("平均打饭耗时"), spinAvgServiceTime_);

    spinServiceStddev_ = new QDoubleSpinBox;
    spinServiceStddev_->setRange(0.0, 120.0);
    spinServiceStddev_->setDecimals(1);
    formFlow->addRow(QStringLiteral("打饭耗时标准差"), spinServiceStddev_);

    spinAvgDiningTime_ = new QSpinBox;
    spinAvgDiningTime_->setRange(60, 7200);
    spinAvgDiningTime_->setSuffix(QStringLiteral(" 秒"));
    formFlow->addRow(QStringLiteral("平均就餐时长"), spinAvgDiningTime_);

    spinDiningStddev_ = new QDoubleSpinBox;
    spinDiningStddev_->setRange(0.0, 1800.0);
    spinDiningStddev_->setDecimals(1);
    formFlow->addRow(QStringLiteral("就餐时长标准差"), spinDiningStddev_);

    // 右列：行为模式
    auto* groupMode = new QGroupBox(QStringLiteral("行为模式"));
    auto* formMode = new QFormLayout(groupMode);

    comboArrivalPattern_ = new QComboBox;
    comboArrivalPattern_->addItem(QStringLiteral("Steady（稳定到达）"),
                                  static_cast<int>(bdss::core::ArrivalPattern::Steady));
    comboArrivalPattern_->addItem(QStringLiteral("RushHour（高峰倍率）"),
                                  static_cast<int>(bdss::core::ArrivalPattern::RushHour));
    formMode->addRow(QStringLiteral("到达模式"), comboArrivalPattern_);

    comboWindowEfficiency_ = new QComboBox;
    comboWindowEfficiency_->addItem(QStringLiteral("Uniform（窗口效率一致）"),
                                    static_cast<int>(bdss::core::WindowEfficiency::Uniform));
    comboWindowEfficiency_->addItem(QStringLiteral("Variable（快慢窗口混合）"),
                                    static_cast<int>(bdss::core::WindowEfficiency::Variable));
    formMode->addRow(QStringLiteral("窗口效率"), comboWindowEfficiency_);

    spinRushStart_ = new QSpinBox;
    spinRushStart_->setRange(0, 24 * 3600);
    spinRushStart_->setSuffix(QStringLiteral(" 秒"));
    formMode->addRow(QStringLiteral("高峰开始（秒）"), spinRushStart_);

    spinRushEnd_ = new QSpinBox;
    spinRushEnd_->setRange(0, 24 * 3600);
    spinRushEnd_->setSuffix(QStringLiteral(" 秒"));
    formMode->addRow(QStringLiteral("高峰结束（秒）"), spinRushEnd_);

    spinRushMultiplier_ = new QDoubleSpinBox;
    spinRushMultiplier_->setRange(1.0, 10.0);
    spinRushMultiplier_->setDecimals(2);
    formMode->addRow(QStringLiteral("高峰倍率"), spinRushMultiplier_);

    auto* btnLoadDefault = new QPushButton(QStringLiteral("恢复默认"));
    connect(btnLoadDefault, &QPushButton::clicked, this, &MainWindow::onLoadDefaults);
    formMode->addRow(btnLoadDefault);

    root->addWidget(groupScale);
    root->addWidget(groupFlow);
    root->addWidget(groupMode);
    return page;
}

QWidget* MainWindow::buildLiveTab() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);

    renderWidget_ = new RenderWidget(page);
    renderWidget_->setMinimumHeight(440);

    // 数字面板：6 个统计指标
    auto* panel = new QGroupBox(QStringLiteral("实时指标"));
    auto* grid = new QGridLayout(panel);

    auto addStat = [&](int row, int col, const QString& title, QLabel*& target) {
        auto* box = new QWidget;
        auto* v = new QVBoxLayout(box);
        v->setContentsMargins(8, 4, 8, 4);
        auto* t = new QLabel(title);
        t->setAlignment(Qt::AlignCenter);
        t->setStyleSheet("color:#666;");
        target = makeStatValue();
        v->addWidget(t);
        v->addWidget(target);
        grid->addWidget(box, row, col);
    };

    addStat(0, 0, QStringLiteral("仿真时间"), statTime_);
    addStat(0, 1, QStringLiteral("窗口排队总数"), statQueue_);
    addStat(0, 2, QStringLiteral("等座人数"), statWaitSeat_);
    addStat(0, 3, QStringLiteral("已就座"), statOccupied_);
    addStat(0, 4, QStringLiteral("座位利用率"), statUtilization_);
    addStat(0, 5, QStringLiteral("已离开学生"), statFinished_);

    layout->addWidget(renderWidget_, /*stretch=*/1);
    layout->addWidget(panel);
    return page;
}

QWidget* MainWindow::buildChartsTab() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    auto* hint = new QLabel(QStringLiteral(
        "📊 统计图表页（下一阶段实现）\n\n"
        "将基于 StatisticsLogger 的 TickRecord 队列长度、等座人数、座位利用率随时间变化曲线，\n"
        "并展示仿真结束后的汇总指标。"));
    hint->setAlignment(Qt::AlignCenter);
    hint->setStyleSheet("color:#888; font-size:14px;");
    layout->addStretch();
    layout->addWidget(hint);
    layout->addStretch();
    return page;
}

void MainWindow::onLoadDefaults() {
    bdss::core::Config config;
    namespace fs = std::filesystem;
    const fs::path defaults{"resources/default_config.json"};
    if (fs::exists(defaults)) {
        config = bdss::core::Config::loadFromFile(defaults);
    }
    writeConfigToForm(config);
    statusBar()->showMessage(QStringLiteral("已恢复默认参数"), 2000);
}

void MainWindow::writeConfigToForm(const bdss::core::Config& config) {
    spinWindowCount_->setValue(config.windowCount);
    spinTableRows_->setValue(config.tableRows);
    spinTableCols_->setValue(config.tableCols);
    spinTotalSimTime_->setValue(config.totalSimulationTime);
    spinArrivalRate_->setValue(config.arrivalRate);
    spinAvgServiceTime_->setValue(config.avgServiceTime);
    spinAvgDiningTime_->setValue(config.avgDiningTime);
    spinServiceStddev_->setValue(config.serviceStddev);
    spinDiningStddev_->setValue(config.diningStddev);
    spinRushStart_->setValue(config.rushHourStart);
    spinRushEnd_->setValue(config.rushHourEnd);
    spinRushMultiplier_->setValue(config.rushHourMultiplier);
    spinRandomSeed_->setValue(static_cast<int>(config.randomSeed));

    comboArrivalPattern_->setCurrentIndex(
        config.arrivalPattern == bdss::core::ArrivalPattern::RushHour ? 1 : 0);
    comboWindowEfficiency_->setCurrentIndex(
        config.windowEfficiency == bdss::core::WindowEfficiency::Variable ? 1 : 0);
}

bdss::core::Config MainWindow::readConfigFromForm() const {
    bdss::core::Config config;
    config.windowCount = spinWindowCount_->value();
    config.tableRows = spinTableRows_->value();
    config.tableCols = spinTableCols_->value();
    config.totalSimulationTime = spinTotalSimTime_->value();
    config.arrivalRate = spinArrivalRate_->value();
    config.avgServiceTime = spinAvgServiceTime_->value();
    config.avgDiningTime = spinAvgDiningTime_->value();
    config.serviceStddev = spinServiceStddev_->value();
    config.diningStddev = spinDiningStddev_->value();
    config.rushHourStart = spinRushStart_->value();
    config.rushHourEnd = spinRushEnd_->value();
    config.rushHourMultiplier = spinRushMultiplier_->value();
    config.randomSeed = static_cast<unsigned int>(spinRandomSeed_->value());

    config.arrivalPattern = static_cast<bdss::core::ArrivalPattern>(
        comboArrivalPattern_->currentData().toInt());
    config.windowEfficiency = static_cast<bdss::core::WindowEfficiency>(
        comboWindowEfficiency_->currentData().toInt());
    return config;
}

void MainWindow::recreateEngine() {
    const auto config = readConfigFromForm();
    engine_ = std::make_unique<bdss::core::SimulationEngine>(config);
    renderWidget_->setEngine(engine_.get());
    refreshLiveStats();
    renderWidget_->update();
}

void MainWindow::onStartClicked() {
    if (!engine_ || engine_->isFinished()) {
        recreateEngine();
    }
    timer_->start();
    setRunningUi(true);
    tabs_->setCurrentIndex(1);
    statusBar()->showMessage(QStringLiteral("仿真运行中…"));
}

void MainWindow::onPauseClicked() {
    timer_->stop();
    setRunningUi(false);
    statusBar()->showMessage(QStringLiteral("已暂停"));
}

void MainWindow::onResetClicked() {
    timer_->stop();
    engine_.reset();
    renderWidget_->setEngine(nullptr);
    renderWidget_->update();
    statTime_->setText("--");
    statQueue_->setText("--");
    statWaitSeat_->setText("--");
    statOccupied_->setText("--");
    statUtilization_->setText("--");
    statFinished_->setText("--");
    setRunningUi(false);
    statusBar()->showMessage(QStringLiteral("已重置，等待新一次仿真"));
}

void MainWindow::onStepClicked() {
    if (!engine_ || engine_->isFinished()) {
        recreateEngine();
    }
    if (!engine_->isFinished()) {
        engine_->tick();
    }
    refreshLiveStats();
    renderWidget_->update();
}

void MainWindow::onSpeedChanged(int value) {
    const int idx = std::clamp(value, 0, static_cast<int>(kSpeedSteps.size()) - 1);
    ticksPerFrame_ = kSpeedSteps[idx];
    if (labelSpeed_) {
        labelSpeed_->setText(QString(QStringLiteral("速度: %1×")).arg(ticksPerFrame_));
    }
}

void MainWindow::onTick() {
    if (!engine_) {
        timer_->stop();
        return;
    }

    for (int i = 0; i < ticksPerFrame_ && !engine_->isFinished(); ++i) {
        engine_->tick();
    }

    refreshLiveStats();
    renderWidget_->update();

    if (engine_->isFinished()) {
        timer_->stop();
        setRunningUi(false);
        statusBar()->showMessage(QStringLiteral("仿真结束。已完成学生：%1")
                                     .arg(engine_->getStatistics().getFinishedCount()));
    }
}

void MainWindow::refreshLiveStats() {
    if (!engine_) {
        return;
    }
    statTime_->setText(QString("%1 / %2")
                           .arg(humanizeSeconds(engine_->getCurrentTime()))
                           .arg(humanizeSeconds(engine_->getTotalSimulationTime())));
    statQueue_->setText(QString::number(engine_->getTotalQueueLength()));
    statWaitSeat_->setText(QString::number(engine_->getWaitingForSeatCount()));
    statOccupied_->setText(QString("%1 / %2")
                               .arg(engine_->getOccupiedSeats())
                               .arg(engine_->getTotalSeats()));
    statUtilization_->setText(QString::number(engine_->getSeatUtilization() * 100.0, 'f', 1) + "%");
    statFinished_->setText(QString::number(engine_->getStatistics().getFinishedCount()));
}

void MainWindow::setRunningUi(bool running) {
    btnStart_->setEnabled(!running);
    btnPause_->setEnabled(running);
    btnStep_->setEnabled(!running);
    btnReset_->setEnabled(true);

    // 运行中禁用参数表单，避免热修改导致引擎崩溃
    const QWidget* const formWidgetsConst[] = {
        spinWindowCount_, spinTableRows_, spinTableCols_, spinTotalSimTime_,
        spinAvgServiceTime_, spinAvgDiningTime_, spinRushStart_, spinRushEnd_,
        spinRandomSeed_, spinArrivalRate_, spinServiceStddev_, spinDiningStddev_,
        spinRushMultiplier_, comboArrivalPattern_, comboWindowEfficiency_
    };
    for (const QWidget* w : formWidgetsConst) {
        const_cast<QWidget*>(w)->setEnabled(!running);
    }
}

} // namespace bdss::gui
