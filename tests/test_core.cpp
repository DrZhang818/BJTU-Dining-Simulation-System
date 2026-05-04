#include "core/Canteen.h"
#include "core/Config.h"
#include "core/SimulationEngine.h"
#include "core/Student.h"
#include "core/Window.h"
#include "utils/RandomGenerator.h"

#include <cassert>
#include <iostream>
#include <memory>

using bdss::core::Canteen;
using bdss::core::Config;
using bdss::core::SimulationEngine;
using bdss::core::Student;
using bdss::core::StudentState;
using bdss::core::Window;

void testStudent() {
    Student student(1, 0, 3, 5);

    assert(student.getId() == 1);
    assert(student.getState() == StudentState::Arrived);
    assert(student.getRemainingServiceTime() == 3);
    assert(student.getRemainingDiningTime() == 5);

    student.startQueuing(0);
    student.startServing(2);
    assert(student.getWaitTime() == 2);

    student.finishServiceAndWaitForSeat(5);
    student.startDining(7);
    assert(student.getSeatWaitTime() == 2);

    student.finishDiningAndLeave(12);
    assert(student.getTotalTime() == 12);

    std::cout << "[PASS] Student state and time calculation" << '\n';
}

void testWindow() {
    Window window(1, 1.0);
    auto student = std::make_shared<Student>(1, 0, 3, 5);

    window.enqueue(student, 0);
    assert(window.getQueueLength() == 1);

    assert(window.tick(0) == nullptr);
    assert(window.tick(1) == nullptr);

    auto completed = window.tick(2);
    assert(completed != nullptr);
    assert(completed->getState() == StudentState::WaitingForSeat);
    assert(window.empty());

    std::cout << "[PASS] Window queue and service" << '\n';
}

void testCanteen() {
    Canteen canteen(2, 2);
    auto student = std::make_shared<Student>(1, 0, 1, 3);

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

void testRandomGenerator() {
    bdss::utils::RandomGenerator::setSeed(42);

    const int value = bdss::utils::RandomGenerator::getUniformInt(1, 10);
    assert(value >= 1 && value <= 10);

    const int count = bdss::utils::RandomGenerator::getPoisson(5.0);
    assert(count >= 0);

    std::cout << "[PASS] RandomGenerator range test" << '\n';
}

void testSimulationEngine() {
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
    assert(engine.getStatistics().getFinishedCount() > 0);

    engine.getStatistics().printSummary(std::cout);
    engine.getStatistics().exportCSV("simulation_log.csv");

    std::cout << "[PASS] SimulationEngine full-process test" << '\n';
    std::cout << "[INFO] CSV exported to simulation_log.csv" << '\n';
}

int main() {
    testStudent();
    testWindow();
    testCanteen();
    testRandomGenerator();
    testSimulationEngine();

    std::cout << "\nAll core tests passed." << '\n';
    return 0;
}