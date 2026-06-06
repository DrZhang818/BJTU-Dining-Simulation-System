#include "gui/MainWindow.h"

#include "core/Config.h"

#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollArea>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

#if defined(BDSS_HAS_CHARTS)
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#endif

#include <algorithm>
#include <exception>
#include <fstream>

namespace bdss::gui {

namespace {

constexpr int kTimerIntervalMs = 50; // 20 FPS
// 仿真 0 秒 = 11:00
constexpr int kBaseHour = 11;

// QTime → 仿真秒（相对于 11:00）
int timeToSeconds(const QTime& t) {
    const int baseMin = kBaseHour * 60;
    const int tMin = t.hour() * 60 + t.minute();
    return std::max(0, (tMin - baseMin) * 60);
}

// 仿真秒 → QTime
QTime secondsToTime(int seconds) {
    const int totalMin = kBaseHour * 60 + seconds / 60;
    return QTime(totalMin / 60, totalMin % 60);
}

QLabel* makeStatValue(const QString& initial = QStringLiteral("--")) {
    auto* label = new QLabel(initial);
    label->setStyleSheet("font-weight: bold; color: #1976D2; font-size: 14px;");
    label->setAlignment(Qt::AlignCenter);
    return label;
}

QString humanizeSeconds(int totalSeconds) {
    if (totalSeconds < 0) return "--";
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
    setWindowTitle(QStringLiteral("BDSS · 北京交通大学就餐仿真系统 v2.1"));
    resize(960, 720);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(6, 6, 6, 6);

    layout->addWidget(buildControlBar());

    tabs_ = new QTabWidget(central);
    tabs_->addTab(buildConfigTab(), QStringLiteral("① 参数配置"));
    tabs_->addTab(buildLiveTab(),   QStringLiteral("② 实时监控"));
    tabs_->addTab(buildChartsTab(), QStringLiteral("③ 统计图表"));
    tabs_->setCurrentIndex(0);
    layout->addWidget(tabs_, 1);

    setCentralWidget(central);

    connect(timer_, &QTimer::timeout, this, &MainWindow::onTick);
    timer_->setInterval(kTimerIntervalMs);

    setRunningUi(false);
    writeConfigToForm(core::Config{});
    statusBar()->showMessage(QStringLiteral("就绪。请在参数配置页设置后点击「开始」"));
}

QWidget* MainWindow::buildControlBar() {
    auto* bar = new QWidget;
    auto* layout = new QHBoxLayout(bar);
    layout->setContentsMargins(4, 4, 4, 4);

    btnStart_ = new QPushButton(QStringLiteral("▶ 开始"));
    btnPause_ = new QPushButton(QStringLiteral("⏸ 暂停"));
    btnStep_  = new QPushButton(QStringLiteral("⏭ 单步"));
    btnReset_ = new QPushButton(QStringLiteral("↺ 重置"));
    btnExportCsv_ = new QPushButton(QStringLiteral("📥 导出CSV"));
    btnExportCsv_->setEnabled(false);

    for (auto* b : {btnStart_, btnPause_, btnStep_, btnReset_, btnExportCsv_}) {
        b->setMinimumHeight(30);
    }

    connect(btnStart_, &QPushButton::clicked, this, &MainWindow::startSimulation);
    connect(btnPause_, &QPushButton::clicked, [this]() {
        timer_->stop();
        setRunningUi(false);
        statusBar()->showMessage(QStringLiteral("已暂停"));
    });
    connect(btnStep_,  &QPushButton::clicked, this, &MainWindow::stepSimulation);
    connect(btnReset_, &QPushButton::clicked, this, &MainWindow::resetSimulation);
    connect(btnExportCsv_, &QPushButton::clicked, this, &MainWindow::exportCsv);

    layout->addWidget(btnStart_);
    layout->addWidget(btnPause_);
    layout->addWidget(btnStep_);
    layout->addWidget(btnReset_);
    layout->addSpacing(12);
    layout->addWidget(btnExportCsv_);
    layout->addStretch();

    return bar;
}

QWidget* MainWindow::buildConfigTab() {
    auto* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);

    auto* page = new QWidget;
    auto* root = new QHBoxLayout(page);

    // === 左列：仿真规模 ===
    auto* groupScale = new QGroupBox(QStringLiteral("仿真规模"));
    auto* formScale = new QFormLayout(groupScale);
    spinWindowCount_ = new QSpinBox; spinWindowCount_->setRange(1, 50);
    formScale->addRow(QStringLiteral("窗口数量"), spinWindowCount_);
    spinTableRows_ = new QSpinBox; spinTableRows_->setRange(1, 50);
    formScale->addRow(QStringLiteral("餐桌行数"), spinTableRows_);
    spinTableCols_ = new QSpinBox; spinTableCols_->setRange(1, 50);
    formScale->addRow(QStringLiteral("餐桌列数"), spinTableCols_);
    spinTotalSimTime_ = new QSpinBox; spinTotalSimTime_->setRange(60, 36000);
    spinTotalSimTime_->setSuffix(QStringLiteral(" 秒"));
    formScale->addRow(QStringLiteral("仿真总时长"), spinTotalSimTime_);
    spinRandomSeed_ = new QSpinBox; spinRandomSeed_->setRange(0, 999999);
    formScale->addRow(QStringLiteral("随机种子"), spinRandomSeed_);
    root->addWidget(groupScale);

    // === 中列：人流与服务 ===
    auto* groupFlow = new QGroupBox(QStringLiteral("人流与服务"));
    auto* formFlow = new QFormLayout(groupFlow);
    spinArrivalRate_ = new QDoubleSpinBox; spinArrivalRate_->setRange(0.1, 200.0);
    spinArrivalRate_->setDecimals(1);
    spinArrivalRate_->setSuffix(QStringLiteral(" 人/分钟"));
    formFlow->addRow(QStringLiteral("平均到达率"), spinArrivalRate_);
    spinAvgServiceTime_ = new QSpinBox; spinAvgServiceTime_->setRange(1, 300);
    spinAvgServiceTime_->setSuffix(QStringLiteral(" 秒"));
    formFlow->addRow(QStringLiteral("平均打饭耗时"), spinAvgServiceTime_);
    spinServiceStddev_ = new QDoubleSpinBox; spinServiceStddev_->setRange(0.0, 120.0);
    spinServiceStddev_->setDecimals(1);
    formFlow->addRow(QStringLiteral("打饭耗时标准差"), spinServiceStddev_);
    spinAvgDiningTime_ = new QSpinBox; spinAvgDiningTime_->setRange(60, 7200);
    spinAvgDiningTime_->setSuffix(QStringLiteral(" 秒"));
    formFlow->addRow(QStringLiteral("平均就餐时长"), spinAvgDiningTime_);
    spinDiningStddev_ = new QDoubleSpinBox; spinDiningStddev_->setRange(0.0, 1800.0);
    spinDiningStddev_->setDecimals(1);
    formFlow->addRow(QStringLiteral("就餐时长标准差"), spinDiningStddev_);

    auto* groupPeak = new QGroupBox(QStringLiteral("高峰时段"));
    auto* peakLayout = new QVBoxLayout(groupPeak);

    // 时间偏移基准：仿真 0s = 11:00
    auto makePeakForm = [](const QString& label, QTimeEdit*& start, QTimeEdit*& end, QDoubleSpinBox*& mult) {
        auto* group = new QGroupBox(label);
        auto* form = new QFormLayout(group);
        start = new QTimeEdit; start->setDisplayFormat(QStringLiteral("HH:mm"));
        start->setTimeRange(QTime(11, 0), QTime(23, 59));
        form->addRow(QStringLiteral("开始时间"), start);
        end = new QTimeEdit; end->setDisplayFormat(QStringLiteral("HH:mm"));
        end->setTimeRange(QTime(11, 0), QTime(23, 59));
        form->addRow(QStringLiteral("结束时间"), end);
        mult = new QDoubleSpinBox; mult->setRange(1.0, 10.0); mult->setDecimals(1);
        form->addRow(QStringLiteral("倍率"), mult);
        return group;
    };

    peakLayout->addWidget(makePeakForm(QStringLiteral("第一段高峰（如 12:00-12:15）"),
                                        timeRush1Start_, timeRush1End_, spinRush1Mult_));
    peakLayout->addWidget(makePeakForm(QStringLiteral("第二段高峰（如 12:20-12:40）"),
                                        timeRush2Start_, timeRush2End_, spinRush2Mult_));
    root->addWidget(groupFlow);
    root->addWidget(groupPeak);

    // === 右列：偏好系统 ===
    auto* groupPref = new QGroupBox(QStringLiteral("偏好系统（画像 + 窗口偏好）"));
    auto* formPref = new QFormLayout(groupPref);

    checkEnablePreferences_ = new QCheckBox(QStringLiteral("开启就餐者偏好"));
    formPref->addRow(checkEnablePreferences_);

    auto* groupProf = new QGroupBox(QStringLiteral("画像比例"));
    auto* formProf = new QFormLayout(groupProf);
    spinRatioUndergrad_ = new QDoubleSpinBox; spinRatioUndergrad_->setRange(0.0, 1.0);
    spinRatioUndergrad_->setDecimals(2); spinRatioUndergrad_->setSingleStep(0.05);
    formProf->addRow(QStringLiteral("本科生比例"), spinRatioUndergrad_);
    spinRatioGrad_ = new QDoubleSpinBox; spinRatioGrad_->setRange(0.0, 1.0);
    spinRatioGrad_->setDecimals(2); spinRatioGrad_->setSingleStep(0.05);
    formProf->addRow(QStringLiteral("研究生比例"), spinRatioGrad_);
    spinRatioStaff_ = new QDoubleSpinBox; spinRatioStaff_->setRange(0.0, 1.0);
    spinRatioStaff_->setDecimals(2); spinRatioStaff_->setSingleStep(0.05);
    formProf->addRow(QStringLiteral("教职工比例"), spinRatioStaff_);
    formPref->addRow(groupProf);

    auto* groupDining = new QGroupBox(QStringLiteral("就餐时长倍率"));
    auto* formDining = new QFormLayout(groupDining);
    spinUndergradDiningFactor_ = new QDoubleSpinBox; spinUndergradDiningFactor_->setRange(0.5, 2.0);
    spinUndergradDiningFactor_->setDecimals(2);
    formDining->addRow(QStringLiteral("本科生倍率"), spinUndergradDiningFactor_);
    spinStaffDiningFactor_ = new QDoubleSpinBox; spinStaffDiningFactor_->setRange(0.5, 2.0);
    spinStaffDiningFactor_->setDecimals(2);
    formDining->addRow(QStringLiteral("教职工倍率"), spinStaffDiningFactor_);
    formPref->addRow(groupDining);

    auto* groupPrefW = new QGroupBox(QStringLiteral("选窗偏好"));
    auto* formPrefW = new QFormLayout(groupPrefW);
    spinQueueLengthWeight_ = new QDoubleSpinBox; spinQueueLengthWeight_->setRange(0.1, 10.0);
    spinQueueLengthWeight_->setDecimals(1);
    formPrefW->addRow(QStringLiteral("队列权重"), spinQueueLengthWeight_);
    spinPreferenceBonus_ = new QDoubleSpinBox; spinPreferenceBonus_->setRange(0.0, 20.0);
    spinPreferenceBonus_->setDecimals(1);
    formPrefW->addRow(QStringLiteral("偏好加分"), spinPreferenceBonus_);
    formPref->addRow(groupPrefW);

    root->addWidget(groupPref);

    // === Phase 2 参数 ===
    auto* groupPhase2 = new QGroupBox(QStringLiteral("Phase 2 扩展功能"));
    auto* formPhase2 = new QFormLayout(groupPhase2);

    // 排队耐心
    auto* groupPat = new QGroupBox(QStringLiteral("排队耐心模型"));
    auto* formPat = new QFormLayout(groupPat);
    checkEnablePatience_ = new QCheckBox(QStringLiteral("开启排队耐心"));
    formPat->addRow(checkEnablePatience_);
    spinPatienceBase_ = new QDoubleSpinBox; spinPatienceBase_->setRange(10, 600);
    spinPatienceBase_->setDecimals(0); spinPatienceBase_->setSuffix(QStringLiteral(" 秒"));
    formPat->addRow(QStringLiteral("平均耐心"), spinPatienceBase_);
    spinPatienceStddev_ = new QDoubleSpinBox; spinPatienceStddev_->setRange(0, 300);
    spinPatienceStddev_->setDecimals(1);
    formPat->addRow(QStringLiteral("耐心标准差"), spinPatienceStddev_);
    checkPatienceAllowSwitch_ = new QCheckBox(QStringLiteral("允许换队"));
    formPat->addRow(checkPatienceAllowSwitch_);
    formPhase2->addRow(groupPat);

    // 打包/堂食
    auto* groupPack = new QGroupBox(QStringLiteral("打包/堂食模式"));
    auto* formPack = new QFormLayout(groupPack);
    checkEnablePacking_ = new QCheckBox(QStringLiteral("开启打包模式"));
    formPack->addRow(checkEnablePacking_);
    spinPackingRatio_ = new QDoubleSpinBox; spinPackingRatio_->setRange(0.0, 1.0);
    spinPackingRatio_->setDecimals(2); spinPackingRatio_->setSingleStep(0.05);
    formPack->addRow(QStringLiteral("打包比例"), spinPackingRatio_);
    spinPackingServiceFactor_ = new QDoubleSpinBox; spinPackingServiceFactor_->setRange(1.0, 3.0);
    spinPackingServiceFactor_->setDecimals(1);
    formPack->addRow(QStringLiteral("打包服务倍率"), spinPackingServiceFactor_);
    formPhase2->addRow(groupPack);

    // 清洁
    auto* groupClean = new QGroupBox(QStringLiteral("座位翻台/清洁"));
    auto* formClean = new QFormLayout(groupClean);
    checkEnableCleaning_ = new QCheckBox(QStringLiteral("开启座位清洁"));
    formClean->addRow(checkEnableCleaning_);
    spinCleaningTime_ = new QSpinBox; spinCleaningTime_->setRange(0, 300);
    spinCleaningTime_->setSuffix(QStringLiteral(" 秒"));
    formClean->addRow(QStringLiteral("清洁时长"), spinCleaningTime_);
    formPhase2->addRow(groupClean);

    // 结伴就餐
    auto* groupGroup = new QGroupBox(QStringLiteral("结伴就餐"));
    auto* formGroup = new QFormLayout(groupGroup);
    checkEnableGroupDining_ = new QCheckBox(QStringLiteral("开启结伴就餐"));
    formGroup->addRow(checkEnableGroupDining_);
    spinGroupRatio_ = new QDoubleSpinBox; spinGroupRatio_->setRange(0.0, 1.0);
    spinGroupRatio_->setDecimals(2); spinGroupRatio_->setSingleStep(0.05);
    formGroup->addRow(QStringLiteral("结伴比例"), spinGroupRatio_);
    spinGroupSizeMin_ = new QSpinBox; spinGroupSizeMin_->setRange(2, 10);
    formGroup->addRow(QStringLiteral("最小组员数"), spinGroupSizeMin_);
    spinGroupSizeMax_ = new QSpinBox; spinGroupSizeMax_->setRange(2, 10);
    formGroup->addRow(QStringLiteral("最大组员数"), spinGroupSizeMax_);
    formPhase2->addRow(groupGroup);

    // 座位偏好
    auto* groupSeat = new QGroupBox(QStringLiteral("座位偏好选择"));
    auto* formSeat = new QFormLayout(groupSeat);
    checkEnableSeatPreference_ = new QCheckBox(QStringLiteral("开启座位偏好"));
    formSeat->addRow(checkEnableSeatPreference_);
    spinWindowProximityWeight_ = new QDoubleSpinBox; spinWindowProximityWeight_->setRange(0.0, 20.0);
    spinWindowProximityWeight_->setDecimals(1); spinWindowProximityWeight_->setSingleStep(0.5);
    formSeat->addRow(QStringLiteral("窗口就近权重"), spinWindowProximityWeight_);
    spinGroupProximityWeight_ = new QDoubleSpinBox; spinGroupProximityWeight_->setRange(0.0, 20.0);
    spinGroupProximityWeight_->setDecimals(1); spinGroupProximityWeight_->setSingleStep(0.5);
    formSeat->addRow(QStringLiteral("结伴邻桌权重"), spinGroupProximityWeight_);
    spinIsolationWeight_ = new QDoubleSpinBox; spinIsolationWeight_->setRange(0.0, 20.0);
    spinIsolationWeight_->setDecimals(1); spinIsolationWeight_->setSingleStep(0.5);
    formSeat->addRow(QStringLiteral("陌生人距离感权重"), spinIsolationWeight_);
    formPhase2->addRow(groupSeat);

    root->addWidget(groupPhase2);
    scrollArea->setWidget(page);
    return scrollArea;
}

QWidget* MainWindow::buildLiveTab() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);

    renderWidget_ = new RenderWidget(page);
    renderWidget_->setMinimumHeight(320);

    // 六指标面板
    auto* panel = new QGroupBox(QStringLiteral("实时指标"));
    auto* grid = new QGridLayout(panel);
    auto addStat = [&](int r, int c, const QString& title, QLabel*& target) {
        auto* box = new QWidget;
        auto* v = new QVBoxLayout(box);
        v->setContentsMargins(6, 2, 6, 2);
        auto* t = new QLabel(title);
        t->setAlignment(Qt::AlignCenter);
        t->setStyleSheet("color:#666;");
        target = makeStatValue();
        v->addWidget(t);
        v->addWidget(target);
        grid->addWidget(box, r, c);
    };
    addStat(0, 0, QStringLiteral("仿真时间"), statTime_);
    addStat(0, 1, QStringLiteral("窗口排队总数"), statQueue_);
    addStat(0, 2, QStringLiteral("等座人数"), statWaitSeat_);
    addStat(0, 3, QStringLiteral("已就座"), statOccupied_);
    addStat(0, 4, QStringLiteral("座位利用率"), statUtilization_);
    addStat(0, 5, QStringLiteral("已离开学生"), statFinished_);

    layout->addWidget(renderWidget_, 1);
    layout->addWidget(panel);
    return page;
}

QWidget* MainWindow::buildChartsTab() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);

#if defined(BDSS_HAS_CHARTS)
    chart_ = new QChart();
    chart_->setTitle(QStringLiteral("仿真统计曲线"));
    chart_->setAnimationOptions(QChart::SeriesAnimations);
    chart_->legend()->setAlignment(Qt::AlignBottom);

    chartView_ = new QChartView(chart_, page);
    chartView_->setRenderHint(QPainter::Antialiasing);
    chartView_->setMinimumHeight(340);
    layout->addWidget(chartView_);

    // 汇总面板
    auto* summaryGroup = new QGroupBox(QStringLiteral("仿真汇总"));
    auto* summaryLayout = new QFormLayout(summaryGroup);
    summaryPanel_ = new QLabel(QStringLiteral("仿真结束后将在此处显示汇总指标。"));
    summaryPanel_->setWordWrap(true);
    summaryPanel_->setStyleSheet("color:#555; font-size:12px;");
    summaryLayout->addRow(summaryPanel_);
    layout->addWidget(summaryGroup);
#else
    auto* placeholder = new QLabel(QStringLiteral(
        "本项目需链接 Qt6 Charts 模块。\n"
        "仿真结束后点击工具栏的「导出CSV」可将数据导出后使用 Excel/WPS 绘制图表。"));
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet("color:#888; font-size:13px;");
    layout->addStretch();
    layout->addWidget(placeholder);
    layout->addStretch();
#endif
    return page;
}

void MainWindow::writeConfigToForm(const core::Config& config) {
    spinWindowCount_->setValue(config.windowCount);
    spinTableRows_->setValue(config.tableRows);
    spinTableCols_->setValue(config.tableCols);
    spinTotalSimTime_->setValue(config.totalSimulationTime);
    spinArrivalRate_->setValue(config.arrivalRate);
    spinAvgServiceTime_->setValue(config.avgServiceTime);
    spinAvgDiningTime_->setValue(config.avgDiningTime);
    spinServiceStddev_->setValue(config.serviceStddev);
    spinDiningStddev_->setValue(config.diningStddev);
    timeRush1Start_->setTime(secondsToTime(config.rushPeaks.size() > 0 ? config.rushPeaks[0].start : 3600));
    timeRush1End_->setTime(secondsToTime(config.rushPeaks.size() > 0 ? config.rushPeaks[0].end : 4500));
    spinRush1Mult_->setValue(config.rushPeaks.size() > 0 ? config.rushPeaks[0].multiplier : 2.5);
    timeRush2Start_->setTime(secondsToTime(config.rushPeaks.size() > 1 ? config.rushPeaks[1].start : 4800));
    timeRush2End_->setTime(secondsToTime(config.rushPeaks.size() > 1 ? config.rushPeaks[1].end : 6000));
    spinRush2Mult_->setValue(config.rushPeaks.size() > 1 ? config.rushPeaks[1].multiplier : 1.8);
    spinRandomSeed_->setValue(static_cast<int>(config.randomSeed));

    // 偏好
    // 偏好
    checkEnablePreferences_->setChecked(config.enablePreferences);
    spinRatioUndergrad_->setValue(config.ratioUndergrad);
    spinRatioGrad_->setValue(config.ratioGrad);
    spinRatioStaff_->setValue(config.ratioStaff);
    spinUndergradDiningFactor_->setValue(config.undergradDiningFactor);
    spinStaffDiningFactor_->setValue(config.staffDiningFactor);
    spinQueueLengthWeight_->setValue(config.queueLengthWeight);
    spinPreferenceBonus_->setValue(config.preferenceBonus);

    // Phase 2
    checkEnablePatience_->setChecked(config.enablePatience);
    spinPatienceBase_->setValue(config.patienceBase);
    spinPatienceStddev_->setValue(config.patienceStddev);
    checkPatienceAllowSwitch_->setChecked(config.patienceAllowSwitch);
    checkEnablePacking_->setChecked(config.enablePacking);
    spinPackingRatio_->setValue(config.packingRatio);
    spinPackingServiceFactor_->setValue(config.packingServiceFactor);
    checkEnableCleaning_->setChecked(config.enableCleaning);
    spinCleaningTime_->setValue(config.cleaningTime);
    checkEnableGroupDining_->setChecked(config.enableGroupDining);
    spinGroupRatio_->setValue(config.groupRatio);
    spinGroupSizeMin_->setValue(config.groupSizeMin);
    spinGroupSizeMax_->setValue(config.groupSizeMax);

    // 座位偏好
    checkEnableSeatPreference_->setChecked(config.enableSeatPreference);
    spinWindowProximityWeight_->setValue(config.windowProximityWeight);
    spinGroupProximityWeight_->setValue(config.groupProximityWeight);
    spinIsolationWeight_->setValue(config.isolationWeight);
}

core::Config MainWindow::readConfigFromForm() const {
    core::Config config;
    config.windowCount = spinWindowCount_->value();
    config.tableRows = spinTableRows_->value();
    config.tableCols = spinTableCols_->value();
    config.totalSimulationTime = spinTotalSimTime_->value();
    config.arrivalRate = spinArrivalRate_->value();
    config.avgServiceTime = spinAvgServiceTime_->value();
    config.avgDiningTime = spinAvgDiningTime_->value();
    config.serviceStddev = spinServiceStddev_->value();
    config.diningStddev = spinDiningStddev_->value();
    config.rushPeaks.clear();
    config.rushPeaks.push_back({timeToSeconds(timeRush1Start_->time()),
                                 timeToSeconds(timeRush1End_->time()),
                                 spinRush1Mult_->value()});
    config.rushPeaks.push_back({timeToSeconds(timeRush2Start_->time()),
                                 timeToSeconds(timeRush2End_->time()),
                                 spinRush2Mult_->value()});
    config.randomSeed = static_cast<unsigned int>(spinRandomSeed_->value());

    config.enablePreferences = checkEnablePreferences_->isChecked();
    config.ratioUndergrad = spinRatioUndergrad_->value();
    config.ratioGrad = spinRatioGrad_->value();
    config.ratioStaff = spinRatioStaff_->value();
    config.undergradDiningFactor = spinUndergradDiningFactor_->value();
    config.staffDiningFactor = spinStaffDiningFactor_->value();
    config.queueLengthWeight = spinQueueLengthWeight_->value();
    config.preferenceBonus = spinPreferenceBonus_->value();

    // 使 windowCategories 长度与 windowCount 一致
    if (config.enablePreferences && !config.windowCategories.empty()) {
        config.windowCategories.resize(static_cast<std::size_t>(config.windowCount));
    }

    // Phase 2 参数
    config.enablePatience = checkEnablePatience_->isChecked();
    config.patienceBase = spinPatienceBase_->value();
    config.patienceStddev = spinPatienceStddev_->value();
    config.patienceAllowSwitch = checkPatienceAllowSwitch_->isChecked();

    config.enablePacking = checkEnablePacking_->isChecked();
    config.packingRatio = spinPackingRatio_->value();
    config.packingServiceFactor = spinPackingServiceFactor_->value();

    config.enableCleaning = checkEnableCleaning_->isChecked();
    config.cleaningTime = spinCleaningTime_->value();

    config.enableGroupDining = checkEnableGroupDining_->isChecked();
    config.groupRatio = spinGroupRatio_->value();
    config.groupSizeMin = spinGroupSizeMin_->value();
    config.groupSizeMax = spinGroupSizeMax_->value();

    // 座位偏好
    config.enableSeatPreference = checkEnableSeatPreference_->isChecked();
    config.windowProximityWeight = spinWindowProximityWeight_->value();
    config.groupProximityWeight = spinGroupProximityWeight_->value();
    config.isolationWeight = spinIsolationWeight_->value();

    return config;
}

void MainWindow::startSimulation() {
    if (!engine_ || engine_->isFinished()) {
        try {
            engine_ = std::make_unique<core::SimulationEngine>(readConfigFromForm());
        } catch (const std::exception& ex) {
            QMessageBox::warning(this, QStringLiteral("配置错误"),
                                 QStringLiteral("无法创建仿真引擎：%1").arg(ex.what()));
            return;
        }
        renderWidget_->setEngine(engine_.get());
        renderWidget_->update();
    }
    timer_->start();
    btnExportCsv_->setEnabled(false);
    setRunningUi(true);
    tabs_->setCurrentIndex(1);
    statusBar()->showMessage(QStringLiteral("仿真运行中…"));
}

void MainWindow::resetSimulation() {
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
    btnExportCsv_->setEnabled(false);
    setRunningUi(false);
    statusBar()->showMessage(QStringLiteral("已重置"));
}

void MainWindow::stepSimulation() {
    if (!engine_ || engine_->isFinished()) {
        try {
            engine_ = std::make_unique<core::SimulationEngine>(readConfigFromForm());
        } catch (const std::exception& ex) {
            QMessageBox::warning(this, QStringLiteral("配置错误"),
                                 QStringLiteral("无法创建仿真引擎：%1").arg(ex.what()));
            return;
        }
        renderWidget_->setEngine(engine_.get());
    }
    if (!engine_->isFinished()) {
        engine_->tick();
        refreshLiveStats();
        renderWidget_->update();
    }
    if (engine_->isFinished()) {
        btnExportCsv_->setEnabled(true);
        updateCharts();
        statusBar()->showMessage(QStringLiteral("仿真结束。已完成学生：%1")
                                     .arg(engine_->getFinishedStudentCount()));
    }
}

void MainWindow::onTick() {
    if (!engine_ || engine_->isFinished()) {
        timer_->stop();
        setRunningUi(false);
        return;
    }

    // 每 tick 多步，提高仿真速度
    static constexpr int kStepsPerFrame = 20;
    for (int i = 0; i < kStepsPerFrame && !engine_->isFinished(); ++i) {
        engine_->tick();
    }

    refreshLiveStats();
    renderWidget_->update();

    if (engine_->isFinished()) {
        timer_->stop();
        setRunningUi(false);
        btnExportCsv_->setEnabled(true);
        updateCharts();
        statusBar()->showMessage(QStringLiteral("仿真结束。已完成学生：%1")
                                     .arg(engine_->getFinishedStudentCount()));
    }
}

void MainWindow::refreshLiveStats() {
    if (!engine_) return;

    int time = engine_->getCurrentTime();

    statTime_->setText(QString("%1 / %2")
                           .arg(humanizeSeconds(time))
                           .arg(humanizeSeconds(engine_->config().totalSimulationTime)));
    statQueue_->setText(QString::number(engine_->getTotalQueueLength()));
    statWaitSeat_->setText(QString::number(engine_->getWaitingForSeatCount()));
    statOccupied_->setText(QString("%1 / %2")
                               .arg(engine_->getOccupiedSeats())
                               .arg(engine_->config().totalSeats()));
    statUtilization_->setText(QString::number(
        engine_->config().totalSeats() > 0
            ? (static_cast<double>(engine_->getOccupiedSeats()) / engine_->config().totalSeats()) * 100.0
            : 0.0, 'f', 1) + "%");
    statFinished_->setText(QString::number(engine_->getFinishedStudentCount()));
}

void MainWindow::refreshView() {
    refreshLiveStats();
    renderWidget_->update();
}

void MainWindow::setRunningUi(bool running) {
    btnStart_->setEnabled(!running);
    btnPause_->setEnabled(running);
    btnStep_->setEnabled(!running);
    btnReset_->setEnabled(true);

    // 运行中锁定参数
    const QWidget* widgets[] = {
        spinWindowCount_, spinTableRows_, spinTableCols_, spinTotalSimTime_,
        spinArrivalRate_, spinAvgServiceTime_, spinAvgDiningTime_,
        spinServiceStddev_, spinDiningStddev_,
        timeRush1Start_, timeRush1End_, spinRush1Mult_,
        timeRush2Start_, timeRush2End_, spinRush2Mult_, spinRandomSeed_,
        checkEnablePreferences_,
        spinRatioUndergrad_, spinRatioGrad_, spinRatioStaff_,
        spinUndergradDiningFactor_, spinStaffDiningFactor_,
        spinQueueLengthWeight_, spinPreferenceBonus_,
        checkEnablePatience_, spinPatienceBase_, spinPatienceStddev_, checkPatienceAllowSwitch_,
        checkEnablePacking_, spinPackingRatio_, spinPackingServiceFactor_,
        checkEnableCleaning_, spinCleaningTime_,
        checkEnableGroupDining_, spinGroupRatio_, spinGroupSizeMin_, spinGroupSizeMax_,
        checkEnableSeatPreference_,
        spinWindowProximityWeight_, spinGroupProximityWeight_, spinIsolationWeight_
    };
    for (auto* w : widgets) {
        const_cast<QWidget*>(w)->setEnabled(!running);
    }
}

void MainWindow::exportCsv() {
    if (!engine_) return;

    const auto path = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出仿真数据"), "simulation_log.csv",
        QStringLiteral("CSV 文件 (*.csv)"));
    if (path.isEmpty()) return;

    try {
        engine_->getStatistics().exportCSV(path.toStdString());
        statusBar()->showMessage(QStringLiteral("CSV 已导出到：%1").arg(path), 5000);
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, QStringLiteral("导出失败"), ex.what());
    }
}

void MainWindow::updateCharts() {
#if defined(BDSS_HAS_CHARTS)
    if (!engine_ || !chart_) return;

    const auto& stats = engine_->getStatistics();
    const auto& snapshots = stats.snapshots();
    if (snapshots.empty()) return;

    // 清除旧系列
    chart_->removeAllSeries();
    // 清除旧坐标轴
    const auto axes = chart_->axes();
    for (auto* axis : axes) {
        chart_->removeAxis(axis);
    }

    // 三条曲线
    auto* queueSeries = new QLineSeries();
    queueSeries->setName(QStringLiteral("排队总人数"));
    queueSeries->setColor(QColor("#E74C3C"));

    auto* waitSeries = new QLineSeries();
    waitSeries->setName(QStringLiteral("等座人数"));
    waitSeries->setColor(QColor("#F39C12"));

    auto* utilSeries = new QLineSeries();
    utilSeries->setName(QStringLiteral("座位利用率(%)"));
    utilSeries->setColor(QColor("#27AE60"));

    double maxQueue = 0;
    double maxWait = 0;
    double maxUtil = 0;

    // 每隔 N 个点采一个，避免数据太多导致渲染卡顿
    const int step = std::max(1, static_cast<int>(snapshots.size()) / 1000);
    for (std::size_t i = 0; i < snapshots.size(); i += static_cast<std::size_t>(step)) {
        const auto& snap = snapshots[i];
        const double t = static_cast<double>(snap.time);
        queueSeries->append(t, snap.totalQueueLength);
        waitSeries->append(t, snap.waitingForSeatCount);
        utilSeries->append(t, snap.seatUtilization * 100.0);
        if (snap.totalQueueLength > maxQueue) maxQueue = snap.totalQueueLength;
        if (snap.waitingForSeatCount > maxWait) maxWait = snap.waitingForSeatCount;
        if (snap.seatUtilization * 100.0 > maxUtil) maxUtil = snap.seatUtilization * 100.0;
    }
    // 保证最后一个点也在
    if (snapshots.size() > 0 && (snapshots.size() - 1) % static_cast<std::size_t>(step) != 0) {
        const auto& last = snapshots.back();
        queueSeries->append(last.time, last.totalQueueLength);
        waitSeries->append(last.time, last.waitingForSeatCount);
        utilSeries->append(last.time, last.seatUtilization * 100.0);
    }

    chart_->addSeries(queueSeries);
    chart_->addSeries(waitSeries);
    chart_->addSeries(utilSeries);

    // X 轴（时间）
    auto* axisX = new QValueAxis();
    axisX->setTitleText(QStringLiteral("仿真时间（秒）"));
    axisX->setLabelFormat("%d");
    axisX->setRange(0, snapshots.back().time);
    chart_->addAxis(axisX, Qt::AlignBottom);
    queueSeries->attachAxis(axisX);
    waitSeries->attachAxis(axisX);
    utilSeries->attachAxis(axisX);

    // Y 轴（人数，左侧）
    auto* axisY = new QValueAxis();
    axisY->setTitleText(QStringLiteral("人数"));
    axisY->setLabelFormat("%d");
    const double yMax = std::max(maxQueue, maxWait) * 1.1;
    axisY->setRange(0, yMax > 0 ? yMax : 10);
    chart_->addAxis(axisY, Qt::AlignLeft);
    queueSeries->attachAxis(axisY);
    waitSeries->attachAxis(axisY);

    // Y 轴（利用率，右侧）
    auto* axisYRight = new QValueAxis();
    axisYRight->setTitleText(QStringLiteral("利用率(%)"));
    axisYRight->setLabelFormat("%.0f");
    axisYRight->setRange(0, 100);
    chart_->addAxis(axisYRight, Qt::AlignRight);
    utilSeries->attachAxis(axisYRight);

    // 汇总面板
    const auto& summary = stats.summary();
    QString summaryText;
    summaryText += QStringLiteral("完成学生人数：%1\n").arg(summary.finishedStudents);
    summaryText += QStringLiteral("最大排队长度：%1\n").arg(summary.maxQueueLength);
    summaryText += QStringLiteral("最大等座人数：%1\n").arg(summary.maxWaitingForSeatCount);
    summaryText += QStringLiteral("平均排队等待时间：%1 秒\n").arg(summary.averageQueueWaitTime, 0, 'f', 1);
    summaryText += QStringLiteral("平均等座等待时间：%1 秒\n").arg(summary.averageSeatWaitTime, 0, 'f', 1);
    summaryText += QStringLiteral("平均座位利用率：%1 %\n").arg(summary.averageSeatUtilization, 0, 'f', 1);
    summaryPanel_->setText(summaryText);

    // 切到图表页
    tabs_->setCurrentIndex(2);
#endif
}

} // namespace bdss::gui
