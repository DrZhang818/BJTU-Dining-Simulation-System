#include "core/Config.h"
#include "core/SimulationEngine.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#if defined(BDSS_HAS_GUI)
#include "gui/MainWindow.h"

#include <QApplication>
#endif

namespace {

struct CliOptions {
    std::filesystem::path configPath = "resources/default_config.json";
    std::filesystem::path outputPath = "simulation_log.csv";
    std::optional<unsigned int> seed;
    std::optional<int> duration;
    bool forceHeadless = false;
    bool help = false;
};

std::filesystem::path defaultConfigPath() {
    std::filesystem::path local = "resources/default_config.json";
    if (std::filesystem::exists(local)) {
        return local;
    }
#if defined(BDSS_SOURCE_DIR)
    std::filesystem::path fromSource = std::filesystem::path(BDSS_SOURCE_DIR) / "resources" / "default_config.json";
    if (std::filesystem::exists(fromSource)) {
        return fromSource;
    }
#endif
    return local;
}

void printHelp(std::ostream& os) {
    os << "BDSS - BJTU Dining Simulation System\n"
       << "Usage:\n"
       << "  BDSS [--headless] [--config <path>] [--output <csv>] [--seed <n>] [--duration <seconds>]\n\n"
       << "Options:\n"
       << "  --headless, --no-gui, -c     Run command-line simulation even when GUI is available\n"
       << "  --config <path>              JSON configuration file, default resources/default_config.json\n"
       << "  --output <path>              CSV output path, default simulation_log.csv\n"
       << "  --seed <n>                   Override random seed\n"
       << "  --duration <seconds>         Override totalSimulationTime\n"
       << "  --help, -h                   Show this help\n";
}

CliOptions parseArgs(int argc, char** argv) {
    CliOptions options;
    options.configPath = defaultConfigPath();
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto requireValue = [&](std::string_view option) -> std::string {
            if (i + 1 >= argc) {
                throw std::invalid_argument(std::string(option) + " requires a value");
            }
            return argv[++i];
        };

        if (arg == "--help" || arg == "-h") {
            options.help = true;
        } else if (arg == "--headless" || arg == "--no-gui" || arg == "-c") {
            options.forceHeadless = true;
        } else if (arg == "--config") {
            options.configPath = requireValue(arg);
        } else if (arg == "--output") {
            options.outputPath = requireValue(arg);
        } else if (arg == "--seed") {
            options.seed = static_cast<unsigned int>(std::stoul(requireValue(arg)));
        } else if (arg == "--duration") {
            options.duration = std::stoi(requireValue(arg));
        } else {
            throw std::invalid_argument("unknown argument: " + arg);
        }
    }
    return options;
}

int runHeadless(const CliOptions& options) {
    auto config = bdss::core::Config::loadFromFile(options.configPath);
    if (options.seed.has_value()) {
        config.randomSeed = *options.seed;
    }
    if (options.duration.has_value()) {
        config.totalSimulationTime = *options.duration;
    }
    config.validate();

    std::cout << "BDSS version " << BDSS_VERSION << '\n'
              << "Config: " << config << '\n';

    bdss::core::SimulationEngine engine(config);
    engine.run();
    engine.getStatistics().printSummary(std::cout);
    engine.getStatistics().exportCSV(options.outputPath);
    std::cout << "CSV exported to " << options.outputPath.string() << '\n';
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const auto options = parseArgs(argc, argv);
        if (options.help) {
            printHelp(std::cout);
            return 0;
        }

#if defined(BDSS_HAS_GUI)
        if (!options.forceHeadless) {
            QApplication app(argc, argv);
            QApplication::setApplicationName("BDSS");
            QApplication::setOrganizationName("BJTU");
            bdss::gui::MainWindow window;
            window.show();
            return app.exec();
        }
#endif
        return runHeadless(options);
    } catch (const std::exception& ex) {
        std::cerr << "Program failed: " << ex.what() << '\n';
        return 1;
    }
}
