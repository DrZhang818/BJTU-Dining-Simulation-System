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

using bdss::core::Canteen;
using bdss::core::Config;
using bdss::core::SimulationEngine;
using bdss::core::Student;
using bdss::core::StudentState;
using bdss::core::Window;

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
    std::cout << "[PASS] RandomGenerator range test" << '\n';
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
    config.windowEfficiency = bdss::core::WindowEfficiency::Variable;

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

int main() {
    testStudentLifecycle();
    testWindowQueueAndService();
    testCanteenSeatAllocationAndRelease();
    testConfigValidationAndParsing();
    testRandomGeneratorRange();
    testSimulationEngineFullProcess();
    testStatisticsLoggerCsvHeader();
    std::cout << "\nAll core tests passed." << '\n';
    return 0;
}
