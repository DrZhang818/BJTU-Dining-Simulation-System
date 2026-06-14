#pragma once

#include "core/Config.h"
#include "utils/StatisticsLogger.h"

#include <filesystem>
#include <string>

namespace bdss::utils {

struct DecisionReportOptions {
    int expectedPeople = 0;
    std::string scenarioName = "默认仿真场景";
    bool includeAiPrompt = true;
};

class DecisionReportGenerator {
public:
    static std::string generateMarkdown(
        const bdss::core::Config& config,
        const StatisticsLogger& logger,
        const DecisionReportOptions& options);

    static void exportMarkdown(
        const std::filesystem::path& filePath,
        const bdss::core::Config& config,
        const StatisticsLogger& logger,
        const DecisionReportOptions& options);
};

} // namespace bdss::utils
