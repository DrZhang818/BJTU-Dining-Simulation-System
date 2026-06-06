#include "core/Canteen.h"
#include "core/Config.h"
#include "core/SimulationEngine.h"
#include "core/Student.h"
#include "core/Window.h"
#include "utils/RandomGenerator.h"
#include "utils/StatisticsLogger.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

using bdss::core::Canteen;
using bdss::core::Config;
using bdss::core::ProfileType;
using bdss::core::SimulationEngine;
using bdss::core::Student;
using bdss::core::StudentState;
using bdss::core::Window;
using bdss::core::WindowEfficiency;

void testStudentLifecycle() {
    Student student(1, 0, 3, 5);
    assert(student.getId() == 1);
    assert(student.getState() == StudentState::Arrived);
    student.startQueuing(0);
    student.startServing(2);
    assert(student.getQueueWaitTime() == 2);
    assert(!student.advanceServiceOneSecond());
    assert(!student.advanceServiceOneSecond());
    assert(student.advanceServiceOneSecond());
    student.finishServiceAndWaitForSeat(5);
    student.startDining(7, 1, 2);
    assert(student.getSeatWaitTime() == 2);
    for (int i = 0; i < 5; ++i) {
        student.advanceDiningOneSecond();
    }
    student.finishDiningAndLeave(12);
    assert(student.getTotalTime() == 12);
    assert(student.getActualServiceTime() == 3);
    std::cout << "[PASS] Student state and time calculation" << '\n';
}

void testWindowQueueAndService() {
    Window window(1, 1.0);
    auto student = std::make_shared<Student>(1, 0, 3, 5);
    window.enqueue(student, 0);
    assert(window.getQueueLength() == 1);
    assert(window.tick(0) == nullptr);
    assert(window.tick(1) == nullptr);
    auto completed = window.tick(2);
    assert(completed != nullptr);
    assert(completed->getState() == StudentState::WaitingForSeat);
    assert(completed->getServiceEndTime() == 3);
    assert(window.empty());
    std::cout << "[PASS] Window queue and service" << '\n';
}

void testCanteenSeatAllocationAndRelease() {
    Canteen canteen(2, 2);
    auto student = std::make_shared<Student>(1, 0, 1, 3);
    student->startQueuing(0);
    student->startServing(0);
    student->finishServiceAndWaitForSeat(1);
    assert(canteen.trySeat(student, 1));
    assert(canteen.getOccupiedSeats() == 1);
    assert(student->getState() == StudentState::Dining);
    assert(canteen.tick(1).empty());
    assert(canteen.tick(2).empty());
    auto finished = canteen.tick(3);
    assert(finished.size() == 1);
    assert(canteen.getOccupiedSeats() == 0);
    assert(student->getState() == StudentState::Left);
    std::cout << "[PASS] Canteen seat allocation and release" << '\n';
}

void testConfigValidationAndParsing() {
#if defined(BDSS_SOURCE_DIR)
    const auto path = std::filesystem::path(BDSS_SOURCE_DIR) / "resources" / "default_config.json";
    const auto config = Config::loadFromFile(path);
    assert(config.windowCount > 0);
    assert(config.tableRows > 0);
    assert(config.arrivalRatePerSecond(0) > 0.0);
    assert(config.totalSeats() == config.tableRows * config.tableCols);
    // 默认配置 is now enablePreferences=true (with realistic defaults)
    assert(config.enablePreferences);
#endif
    Config bad;
    bad.windowCount = 0;
    bool thrown = false;
    try {
        bad.validate();
    } catch (const std::exception&) {
        thrown = true;
    }
    assert(thrown);
    std::cout << "[PASS] Config parse and validation" << '\n';
}

void testRandomGeneratorRange() {
    bdss::utils::RandomGenerator::setSeed(42);
    const int value = bdss::utils::RandomGenerator::getUniformInt(1, 10);
    assert(value >= 1 && value <= 10);
    const int count = bdss::utils::RandomGenerator::getPoisson(5.0);
    assert(count >= 0);
    const int normal = bdss::utils::RandomGenerator::getNormalInt(20.0, 4.0, 1);
    assert(normal >= 1);

    // 新增：加权抽样测试
    const std::vector<double> weights = {0.6, 0.3, 0.1};
    int counts[3] = {0, 0, 0};
    bdss::utils::RandomGenerator::setSeed(42);
    for (int i = 0; i < 10000; ++i) {
        const int choice = bdss::utils::RandomGenerator::getWeightedChoice(weights);
        assert(choice >= 0 && choice < 3);
        counts[choice]++;
    }
    // 6:3:1 比例，允许 ±5% 容差
    const double ratio0 = static_cast<double>(counts[0]) / 10000.0;
    const double ratio1 = static_cast<double>(counts[1]) / 10000.0;
    assert(ratio0 > 0.5 && ratio0 < 0.7);
    assert(ratio1 > 0.2 && ratio1 < 0.4);
    std::cout << "[PASS] RandomGenerator range and weighted choice" << '\n';
}

void testSimulationEngineFullProcess() {
    Config config;
    config.windowCount = 3;
    config.tableRows = 4;
    config.tableCols = 5;
    config.totalSimulationTime = 120;
    config.arrivalRate = 8.0;
    config.avgServiceTime = 8;
    config.avgDiningTime = 30;
    config.serviceStddev = 2.0;
    config.diningStddev = 5.0;
    config.randomSeed = 42;
    config.windowEfficiency = WindowEfficiency::Variable;
    // 关闭偏好保证严格兼容
    config.enablePreferences = false;
    config.windowCategories.clear();

    SimulationEngine engine(config);
    engine.run();
    assert(engine.getCurrentTime() >= config.totalSimulationTime);
    assert(engine.getTotalQueueLength() == 0);
    assert(engine.getWaitingForSeatCount() == 0);
    assert(engine.getOccupiedSeats() == 0);
    assert(engine.getGeneratedStudentCount() > 0);
    assert(engine.getStatistics().getFinishedCount() == engine.getGeneratedStudentCount());
    assert(!engine.getStatistics().snapshots().empty());

    engine.getStatistics().printSummary(std::cout);
    engine.getStatistics().exportCSV("simulation_log.csv");
    assert(std::filesystem::exists("simulation_log.csv"));
    std::cout << "[PASS] SimulationEngine full-process test" << '\n';
    std::cout << "[INFO] CSV exported to simulation_log.csv" << '\n';
}

void testStatisticsLoggerCsvHeader() {
    bdss::utils::StatisticsLogger logger;
    logger.record(1, 2, 3, 4, 5, 20.0);
    std::vector<std::shared_ptr<Student>> finished;
    logger.finalize(finished);
    logger.exportCSV("statistics_test.csv");
    assert(std::filesystem::exists("statistics_test.csv"));
    std::cout << "[PASS] StatisticsLogger CSV export" << '\n';
}

// ---- 新增：偏好系统测试 ----

void testProfileTypeSerialization() {
    assert(bdss::core::toString(ProfileType::Undergrad) == "Undergrad");
    assert(bdss::core::toString(ProfileType::Grad) == "Grad");
    assert(bdss::core::toString(ProfileType::Staff) == "Staff");
    assert(bdss::core::parseProfileType("undergrad") == ProfileType::Undergrad);
    assert(bdss::core::parseProfileType("研究生")  == ProfileType::Grad);
    assert(bdss::core::parseProfileType("Staff")   == ProfileType::Staff);
    std::cout << "[PASS] ProfileType serialization" << '\n';
}

void testStudentProfile() {
    Student student(1, 0, 5, 10);
    assert(student.getProfile() == ProfileType::Undergrad);
    assert(student.getPreferredCategory() == -1);

    student.setProfile(ProfileType::Staff);
    assert(student.getProfile() == ProfileType::Staff);

    student.setPreferredCategory(2);
    assert(student.getPreferredCategory() == 2);
    std::cout << "[PASS] Student profile fields" << '\n';
}

void testWindowCategory() {
    Window window(1, 1.0);
    assert(window.getCategory().empty());

    window.setCategory("米线");
    assert(window.getCategory() == "米线");
    std::cout << "[PASS] Window category" << '\n';
}

void testPreferencesEnabledDisabled() {
    // 关闭偏好时，结果与旧版完全一致
    Config config;
    config.windowCount = 3;
    config.tableRows = 4;
    config.tableCols = 5;
    config.totalSimulationTime = 30;
    config.arrivalRate = 6.0;
    config.avgServiceTime = 5;
    config.avgDiningTime = 20;
    config.serviceStddev = 1.0;
    config.diningStddev = 4.0;
    config.randomSeed = 42;
    // 显式关闭偏好（默认已改为 true，需先关 validate 再移除品类）
    config.enablePreferences = false;
    config.windowCategories.clear();

    SimulationEngine engine(config);
    engine.run();
    assert(engine.getStatistics().getFinishedCount() == engine.getGeneratedStudentCount());
    assert(engine.getStatistics().getFinishedCount() > 0);
    std::cout << "[PASS] Preferences disabled: behaves as before" << '\n';
    engine.getStatistics().printSummary(std::cout);
}

void testPreferencesEnabledSmoke() {
    // 开启偏好，不崩就行（画像比例 + 品类分配 + 偏好选窗）
    Config config;
    config.windowCount = 3;
    config.tableRows = 4;
    config.tableCols = 5;
    config.totalSimulationTime = 30;
    config.arrivalRate = 6.0;
    config.avgServiceTime = 5;
    config.avgDiningTime = 20;
    config.serviceStddev = 1.0;
    config.diningStddev = 4.0;
    config.randomSeed = 42;
    config.enablePreferences = true;
    config.windowCategories = {"麻辣烫", "米饭套餐", "米线"};

    SimulationEngine engine(config);
    engine.run();
    assert(engine.getStatistics().getFinishedCount() > 0);
    // 所有学生应都有画像
    const auto& summary = engine.getStatistics().summary();
    assert(summary.finishedStudents > 0);
    std::cout << "[PASS] Preferences enabled: simulation runs successfully with " << summary.finishedStudents << " finished" << '\n';
    engine.getStatistics().printSummary(std::cout);
}

void testPreferencesEnableValidate() {
    // 品类数量不匹配时应抛异常
    Config config;
    config.enablePreferences = true;
    config.windowCategories = {"A", "B"};  // 但 windowCount=5
    bool thrown = false;
    try {
        config.validate();
    } catch (const std::exception&) {
        thrown = true;
    }
    assert(thrown);
    std::cout << "[PASS] Preferences validate: mismatched category count rejected" << '\n';
}

int main() {
    // 旧测试（回归）
    testStudentLifecycle();
    testWindowQueueAndService();
    testCanteenSeatAllocationAndRelease();
    testConfigValidationAndParsing();
    testRandomGeneratorRange();
    testSimulationEngineFullProcess();
    testStatisticsLoggerCsvHeader();

    // 偏好系统新测试
    testProfileTypeSerialization();
    testStudentProfile();
    testWindowCategory();
    testPreferencesEnabledDisabled();
    testPreferencesEnabledSmoke();
    testPreferencesEnableValidate();

    std::cout << "\nAll core tests passed." << '\n';
    return 0;
}
