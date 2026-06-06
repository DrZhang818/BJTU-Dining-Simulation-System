#include "core/Config.h"
#include "core/SimulationEngine.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "[FAIL] " << message << '\n';
    }
}

template <typename F>
void runTest(const std::string& name, F&& fn) {
    try {
        fn();
        std::cerr << "[ OK ] " << name << '\n';
    } catch (const std::exception& ex) {
        ++failures;
        std::cerr << "[FAIL] " << name << ": unexpected exception: " << ex.what() << '\n';
    }
}

std::filesystem::path sourceDir() {
#ifdef BDSS_SOURCE_DIR
    return std::filesystem::path(BDSS_SOURCE_DIR);
#else
    return std::filesystem::current_path();
#endif
}

bdss::core::Config smallConfig() {
    bdss::core::Config config;
    config.windowCount = 4;
    config.tableRows = 4;
    config.tableCols = 4;
    config.totalSimulationTime = 120;
    config.arrivalRate = 30.0;
    config.avgServiceTime = 5;
    config.serviceStddev = 0.0;
    config.avgDiningTime = 30;
    config.diningStddev = 0.0;
    config.randomSeed = 123;
    config.arrivalPattern = bdss::core::ArrivalPattern::Steady;
    config.windowEfficiency = bdss::core::WindowEfficiency::Uniform;
    config.enableWindowPreferences = false;
    config.enablePatience = false;
    config.enableTakeaway = false;
    config.enableCleaning = false;
    config.enableGroupDining = false;
    config.enableSeatPreference = false;
    return config;
}

void testConfigLoad() {
    const auto path = sourceDir() / "resources" / "default_config.json";
    auto config = bdss::core::Config::loadFromFile(path);
    expect(config.windowCount == 8, "default windowCount should be 8");
    expect(config.rushPeaks.size() == 2, "default config should load two rush peaks");
    expect(config.windowCategories.size() >= 5, "window categories should be loaded");
    expect(config.packingServiceFactor > 1.0, "packing factor should be loaded");
}

void testValidationRejectsBadConfig() {
    bdss::core::Config config;
    config.windowCount = 0;
    bool threw = false;
    try {
        config.validate();
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    expect(threw, "invalid windowCount must be rejected");
}

void testNewWindowsReceiveStudents() {
    auto config = smallConfig();
    config.windowCount = 7;
    config.arrivalRate = 6000.0;
    config.avgServiceTime = 1000;
    config.totalSimulationTime = 2;
    config.randomSeed = 7;

    bdss::core::SimulationEngine engine(config);
    engine.tick();
    const auto queues = engine.getWindowQueueLengths();
    expect(queues.size() == 7, "engine must create exactly seven windows");
    expect(queues[5] > 0, "sixth window should receive queued students after increasing window count");
    expect(queues[6] > 0, "seventh window should receive queued students after increasing window count");
}

void testRunInvariants() {
    auto config = smallConfig();
    bdss::core::SimulationEngine engine(config);
    engine.run();
    const auto summary = engine.getStatistics().getSummary();
    expect(summary.generatedStudents > 0, "simulation should generate students");
    expect(summary.finishedStudents > 0, "simulation should finish at least one student");
    expect(engine.getInSystemCount() == 0, "finished engine should be empty");
    expect(summary.averageSeatUtilization >= 0.0 && summary.averageSeatUtilization <= 1.0, "seat utilization should stay in [0,1]");
}

void testTakeawayDoesNotOccupySeats() {
    auto config = smallConfig();
    config.enableTakeaway = true;
    config.takeawayRate = 1.0;
    config.totalSimulationTime = 30;
    config.arrivalRate = 100.0;
    bdss::core::SimulationEngine engine(config);
    engine.run();
    const auto summary = engine.getStatistics().getSummary();
    expect(summary.takeawayStudents == summary.generatedStudents, "all generated students should be takeaway when takeawayRate=1");
    expect(summary.averageSeatUtilization == 0.0, "takeaway-only scenario should not occupy seats");
}

void testCsvExport() {
    auto config = smallConfig();
    config.totalSimulationTime = 10;
    bdss::core::SimulationEngine engine(config);
    engine.run();
    const auto path = std::filesystem::temp_directory_path() / "bdss_core_test_export.csv";
    engine.getStatistics().exportCsv(path);
    std::ifstream input(path);
    std::string header;
    std::getline(input, header);
    expect(header.find("window_queue_lengths") != std::string::npos, "CSV should contain per-window queue data");
    std::filesystem::remove(path);
}

void testConfigNormalizeFillsWindowProfiles() {
    bdss::core::Config config;
    config.windowCount = 10;
    config.windowCategories.clear();
    config.windowProfiles.clear();
    config.normalize();
    config.validate();
    expect(config.windowCategories.size() == 1, "normalize should provide a fallback window category");
    expect(config.windowProfiles.size() == 10, "normalize should create one window profile per configured window");
}

void testLiveConfigUpdateAddsRoutableWindows() {
    auto config = smallConfig();
    config.windowCount = 4;
    config.arrivalRate = 6000.0;
    config.avgServiceTime = 1000;
    config.totalSimulationTime = 3;
    config.randomSeed = 77;

    bdss::core::SimulationEngine engine(config);
    engine.tick();

    config.windowCount = 6;
    engine.updateConfig(config, true);
    engine.tick();

    const auto queues = engine.getWindowQueueLengths();
    expect(queues.size() == 6, "live update must resize internal window vector");
    expect(queues[4] > 0, "fifth window should receive students after live config update");
    expect(queues[5] > 0, "sixth window should receive students after live config update");
}

} // namespace

int main() {
    runTest("ConfigLoad", testConfigLoad);
    runTest("ConfigNormalizeFillsWindowProfiles", testConfigNormalizeFillsWindowProfiles);
    runTest("ValidationRejectsBadConfig", testValidationRejectsBadConfig);
    runTest("NewWindowsReceiveStudents", testNewWindowsReceiveStudents);
    runTest("LiveConfigUpdateAddsRoutableWindows", testLiveConfigUpdateAddsRoutableWindows);
    runTest("RunInvariants", testRunInvariants);
    runTest("TakeawayDoesNotOccupySeats", testTakeawayDoesNotOccupySeats);
    runTest("CsvExport", testCsvExport);

    if (failures > 0) {
        std::cerr << failures << " test assertion(s) failed.\n";
        return 1;
    }
    std::cerr << "All core tests passed.\n";
    return 0;
}
