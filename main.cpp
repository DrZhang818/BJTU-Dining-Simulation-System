#include "core/Config.h"
#include "core/SimulationEngine.h"

#include <exception>
#include <iostream>
#include <string>

#if defined(BDSS_HAS_GUI)
#include "gui/MainWindow.h"

#include <QApplication>
#include <QLocale>
#endif

namespace {

int runHeadless() {
    try {
        const auto config = bdss::core::Config::loadFromFile("resources/default_config.json");

        bdss::core::SimulationEngine engine(config);
        engine.run();

        engine.getStatistics().printSummary(std::cout);
        engine.getStatistics().exportCSV("simulation_log.csv");

        std::cout << "CSV exported to simulation_log.csv" << '\n';
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Program failed: " << ex.what() << '\n';
        return 1;
    }
}

bool wantsHeadless(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--headless" || arg == "-c" || arg == "--no-gui") {
            return true;
        }
    }
    return false;
}

} // namespace

int main(int argc, char* argv[]) {
#if defined(BDSS_HAS_GUI)
    if (wantsHeadless(argc, argv)) {
        return runHeadless();
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName("BDSS");
    QApplication::setOrganizationName("BJTU");

    bdss::gui::MainWindow window;
    window.show();
    return app.exec();
#else
    (void)argc;
    (void)argv;
    return runHeadless();
#endif
}
