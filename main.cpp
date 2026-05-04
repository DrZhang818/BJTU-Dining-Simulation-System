#include "core/Config.h"
#include "core/SimulationEngine.h"

#include <exception>
#include <iostream>

int main() {
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