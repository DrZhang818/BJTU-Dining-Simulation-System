#include "core/Config.h"
#include "core/SimulationEngine.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef BDSS_HAS_GUI
#include "gui/MainWindow.h"
#include <QApplication>
#endif

namespace {

struct CliOptions {
    bool headless = false;
    bool gui = false;
    bool help = false;
    std::filesystem::path configPath;
    std::filesystem::path outputPath = "simulation_log.csv";
    std::optional<unsigned int> seed;
    std::optional<int> duration;
    std::optional<int> windowCount;
    std::optional<double> arrivalRate;
};

std::filesystem::path defaultConfigPath() {
#ifdef BDSS_SOURCE_DIR
    return std::filesystem::path(BDSS_SOURCE_DIR) / "resources" / "default_config.json";
#else
    return std::filesystem::path("resources") / "default_config.json";
#endif
}

void printHelp(std::ostream& os) {
    os << "BDSS · 北京交通大学就餐仿真系统\n\n"
       << "Usage:\n"
       << "  BDSS --gui [--config resources/default_config.json]\n"
       << "  BDSS --headless [--config FILE] [--output FILE] [--seed N] [--duration SEC]\n\n"
       << "Options:\n"
       << "  --gui                 启动 Qt 图形界面。\n"
       << "  --headless, --no-gui  命令行仿真并输出 CSV。\n"
       << "  --config FILE         读取 JSON 配置。默认 resources/default_config.json。\n"
       << "  --output FILE         CSV 输出路径。默认 simulation_log.csv。\n"
       << "  --seed N              覆盖随机种子。\n"
       << "  --duration SEC        覆盖仿真时长。\n"
       << "  --window-count N      覆盖餐口数量。\n"
       << "  --arrival-rate X      覆盖平均到达率，单位 人/分钟。\n"
       << "  --help                显示帮助。\n";
}

std::string requireValue(int& i, int argc, char** argv, const std::string& option) {
    if (i + 1 >= argc) {
        throw std::invalid_argument(option + " requires a value");
    }
    return argv[++i];
}

CliOptions parseArgs(int argc, char** argv) {
    CliOptions options;
    options.configPath = defaultConfigPath();

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            options.help = true;
        } else if (arg == "--headless" || arg == "--no-gui") {
            options.headless = true;
        } else if (arg == "--gui") {
            options.gui = true;
        } else if (arg == "--config" || arg == "-c") {
            options.configPath = requireValue(i, argc, argv, arg);
        } else if (arg == "--output" || arg == "-o") {
            options.outputPath = requireValue(i, argc, argv, arg);
        } else if (arg == "--seed") {
            options.seed = static_cast<unsigned int>(std::stoul(requireValue(i, argc, argv, arg)));
        } else if (arg == "--duration") {
            options.duration = std::stoi(requireValue(i, argc, argv, arg));
        } else if (arg == "--window-count") {
            options.windowCount = std::stoi(requireValue(i, argc, argv, arg));
        } else if (arg == "--arrival-rate") {
            options.arrivalRate = std::stod(requireValue(i, argc, argv, arg));
        } else {
            throw std::invalid_argument("Unknown argument: " + arg);
        }
    }

    if (!options.headless && !options.gui) {
#ifdef BDSS_HAS_GUI
        options.gui = true;
#else
        options.headless = true;
#endif
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
    if (options.arrivalRate) {
        config.arrivalRate = *options.arrivalRate;
    }
    config.normalize();
    config.validate();
    return config;
}

int runHeadless(const CliOptions& options) {
    const auto config = loadConfigWithOverrides(options);
    bdss::core::SimulationEngine engine(config);
    engine.run();
    engine.getStatistics().exportCsv(options.outputPath);

    std::cout << "BDSS headless simulation finished.\n"
              << "Config: " << options.configPath << '\n'
              << "CSV: " << options.outputPath << "\n\n"
              << engine.getStatistics().summaryText();
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const CliOptions options = parseArgs(argc, argv);
        if (options.help) {
            printHelp(std::cout);
            return 0;
        }

        if (options.gui && !options.headless) {
#ifdef BDSS_HAS_GUI
            QApplication app(argc, argv);
            bdss::gui::MainWindow window(options.configPath);
            window.show();
            return app.exec();
#else
            std::cerr << "GUI is not available in this build. Reconfigure with Qt6 Widgets or use --headless.\n";
            return 2;
#endif
        }

        return runHeadless(options);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n\n";
        printHelp(std::cerr);
        return 1;
    }
}
