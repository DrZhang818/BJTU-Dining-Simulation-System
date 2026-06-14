#include "core/Config.h"
#include "core/SimulationEngine.h"
#include "utils/DecisionReportGenerator.h"

#include <cassert>
#include <filesystem>
#include <string>

int main() {
    bdss::core::Config config;
    config.totalSimulationTime = 300;
    config.windowCount = 4;
    config.totalSeats = 80;
    config.arrivalRate = 5.0;
    config.randomSeed = 7;
    config.normalize();
    config.validate();

    bdss::core::SimulationEngine engine(config);
    engine.run();

    const auto summary = engine.statistics().summary();
    assert(summary.generatedStudents >= 0);
    assert(!engine.statistics().records().empty());

    bdss::utils::DecisionReportOptions options;
    options.expectedPeople = 500;
    auto report = bdss::utils::DecisionReportGenerator::generateMarkdown(config, engine.statistics(), options);
    assert(report.find("食堂就餐仿真 AI 决策报告") != std::string::npos);
    assert(report.find("预计就餐人数") != std::string::npos);

    return 0;
}
