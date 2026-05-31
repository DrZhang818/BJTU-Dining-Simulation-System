#include "gui/MainWindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

namespace bdss::gui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    auto* root = new QWidget(this);
    auto* layout = new QVBoxLayout(root);

    summaryLabel_ = new QLabel(root);
    summaryLabel_->setWordWrap(true);
    renderWidget_ = new RenderWidget(root);
    startButton_ = new QPushButton(QStringLiteral("开始/暂停"), root);
    auto* resetButton = new QPushButton(QStringLiteral("重置"), root);
    auto* stepButton = new QPushButton(QStringLiteral("单步"), root);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(startButton_);
    buttonLayout->addWidget(stepButton);
    buttonLayout->addWidget(resetButton);

    layout->addWidget(summaryLabel_);
    layout->addWidget(renderWidget_, 1);
    layout->addLayout(buttonLayout);
    setCentralWidget(root);
    resize(720, 560);
    setWindowTitle(QStringLiteral("BDSS 北交大食堂就餐仿真系统"));

    timer_ = new QTimer(this);
    timer_->setInterval(25);
    connect(timer_, &QTimer::timeout, this, &MainWindow::stepSimulation);
    connect(startButton_, &QPushButton::clicked, this, &MainWindow::startSimulation);
    connect(resetButton, &QPushButton::clicked, this, &MainWindow::resetSimulation);
    connect(stepButton, &QPushButton::clicked, this, &MainWindow::stepSimulation);

    resetSimulation();
}

core::Config MainWindow::defaultConfig() const {
    core::Config config;
    return config;
}

void MainWindow::startSimulation() {
    if (timer_->isActive()) {
        timer_->stop();
    } else {
        timer_->start();
    }
}

void MainWindow::resetSimulation() {
    engine_ = std::make_unique<core::SimulationEngine>(defaultConfig());
    refreshView();
}

void MainWindow::stepSimulation() {
    if (!engine_) {
        resetSimulation();
        return;
    }
    if (engine_->isFinished()) {
        timer_->stop();
        return;
    }
    engine_->tick();
    refreshView();
}

void MainWindow::refreshView() {
    if (!engine_) {
        return;
    }
    const auto& config = engine_->config();
    summaryLabel_->setText(QStringLiteral("时间：%1 s | 排队：%2 | 等座：%3 | 已占座位：%4 | 已生成学生：%5")
                               .arg(engine_->getCurrentTime())
                               .arg(engine_->getTotalQueueLength())
                               .arg(engine_->getWaitingForSeatCount())
                               .arg(engine_->getOccupiedSeats())
                               .arg(engine_->getGeneratedStudentCount()));
    renderWidget_->setSeatMatrix(config.tableRows, config.tableCols, engine_->getOccupiedSeats());
}

} // namespace bdss::gui
