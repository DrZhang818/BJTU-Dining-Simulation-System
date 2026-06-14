#include "core/Config.h"
#include "core/SimulationEngine.h"
#include "utils/DecisionReportGenerator.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

struct CliOptions {
    bool help = false;
    std::filesystem::path configPath;
    std::filesystem::path outputPath = "simulation_log.csv";
    std::optional<std::filesystem::path> reportPath;
    std::optional<unsigned int> seed;
    std::optional<int> duration;
    std::optional<int> windowCount;
    std::optional<int> totalSeats;
    std::optional<double> arrivalRate;
    std::optional<int> expectedPeople;
    std::string scenarioName = "命令行仿真场景";
};

std::filesystem::path defaultConfigPath() {
#ifdef BDSS_SOURCE_DIR
    return std::filesystem::path(BDSS_SOURCE_DIR) / "resources" / "default_config.json";
#else
    return std::filesystem::path("resources") / "default_config.json";
#endif
}

std::string requireValue(int& i, int argc, char** argv, const std::string& option) {
    if (i + 1 >= argc) {
        throw std::invalid_argument(option + " requires a value");
    }
    return argv[++i];
}

void printHelp(std::ostream& os) {
    os << "BDSS · 北京交通大学食堂就餐仿真系统\n\n"
       << "Usage:\n"
       << "  BDSS --config resources/default_config.json --output simulation_log.csv --people 3500 --report decision_report.md\n\n"
       << "Options:\n"
       << "  --config FILE            读取 JSON 配置，默认 resources/default_config.json。\n"
       << "  --output FILE, -o FILE   输出 CSV，默认 simulation_log.csv。\n"
       << "  --report FILE            输出 AI 决策报告 Markdown。\n"
       << "  --people N               预计就餐人数，用于报告中的放大建议。\n"
       << "  --scenario-name NAME     报告中的场景名称。\n"
       << "  --seed N                 覆盖随机种子。\n"
       << "  --duration SEC           覆盖仿真时长。\n"
       << "  --window-count N         覆盖开放窗口数。\n"
       << "  --seats N                覆盖座位数。\n"
       << "  --arrival-rate X         覆盖基础到达率，单位 人/分钟。\n"
       << "  --help, -h               显示帮助。\n";
}

CliOptions parseArgs(int argc, char** argv) {
    CliOptions options;
    options.configPath = defaultConfigPath();

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            options.help = true;
        } else if (arg == "--headless" || arg == "--no-gui") {
            // Kept for compatibility with the upstream command line style.
        } else if (arg == "--config" || arg == "-c") {
            options.configPath = requireValue(i, argc, argv, arg);
        } else if (arg == "--output" || arg == "-o") {
            options.outputPath = requireValue(i, argc, argv, arg);
        } else if (arg == "--report") {
            options.reportPath = requireValue(i, argc, argv, arg);
        } else if (arg == "--people") {
            options.expectedPeople = std::stoi(requireValue(i, argc, argv, arg));
        } else if (arg == "--scenario-name") {
            options.scenarioName = requireValue(i, argc, argv, arg);
        } else if (arg == "--seed") {
            options.seed = static_cast<unsigned int>(std::stoul(requireValue(i, argc, argv, arg)));
        } else if (arg == "--duration") {
            options.duration = std::stoi(requireValue(i, argc, argv, arg));
        } else if (arg == "--window-count") {
            options.windowCount = std::stoi(requireValue(i, argc, argv, arg));
        } else if (arg == "--seats") {
            options.totalSeats = std::stoi(requireValue(i, argc, argv, arg));
        } else if (arg == "--arrival-rate") {
            options.arrivalRate = std::stod(requireValue(i, argc, argv, arg));
        } else {
            throw std::invalid_argument("Unknown argument: " + arg);
        }
    }

    return options;
}

bdss::core::Config loadConfigWithOverrides(const CliOptions& options) {
    bdss::core::Config config;
    if (!options.configPath.empty() && std::filesystem::exists(options.configPath)) {
        config = bdss::core::Config::loadFromFile(options.configPath);
    } else if (!options.configPath.empty()) {
        throw std::runtime_error("Config file does not exist: " + options.configPath.string());
    }

    if (options.seed) {
        config.randomSeed = *options.seed;
    }
    if (options.duration) {
        config.totalSimulationTime = *options.duration;
    }
    if (options.windowCount) {
        config.windowCount = *options.windowCount;
    }
    if (options.totalSeats) {
        config.totalSeats = *options.totalSeats;
    }
    if (options.arrivalRate) {
        config.arrivalRate = *options.arrivalRate;
    }
    config.normalize();
    config.validate();
    return config;
}

int runSimulation(const CliOptions& options) {
    const auto config = loadConfigWithOverrides(options);
    bdss::core::SimulationEngine engine(config);
    engine.run();

    engine.statistics().exportCsv(options.outputPath);

    std::cout << "BDSS simulation finished.\n"
              << "Config: " << options.configPath << '\n'
              << "CSV: " << options.outputPath << "\n";

    if (options.reportPath) {
        bdss::utils::DecisionReportOptions reportOptions;
        reportOptions.expectedPeople = options.expectedPeople.value_or(0);
        reportOptions.scenarioName = options.scenarioName;
        bdss::utils::DecisionReportGenerator::exportMarkdown(
            *options.reportPath, config, engine.statistics(), reportOptions);
        std::cout << "AI decision report: " << *options.reportPath << "\n";
    }

    std::cout << '\n' << engine.statistics().summaryText();
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto options = parseArgs(argc, argv);
        if (options.help) {
            printHelp(std::cout);
            return 0;
        }
        return runSimulation(options);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n\n";
        printHelp(std::cerr);
        return 1;
    }
}
