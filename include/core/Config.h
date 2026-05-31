#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <string>

namespace bdss::core {

enum class ArrivalPattern {
    Uniform,
    RushHour
};

enum class WindowEfficiency {
    Equal,
    Variable
};

struct Config {
    int windowCount = 5;
    int tableRows = 10;
    int tableCols = 10;
    int totalSimulationTime = 3600;

    // Students per minute. The original project kept the key name "arrivalRate";
    // this implementation treats it as a per-minute intensity to match the
    // expected lunch-hour scale.
    double arrivalRate = 5.0;

    int avgServiceTime = 20;
    int avgDiningTime = 900;
    double serviceStddev = 5.0;
    double diningStddev = 180.0;

    ArrivalPattern arrivalPattern = ArrivalPattern::RushHour;
    WindowEfficiency windowEfficiency = WindowEfficiency::Variable;

    int rushHourStart = 900;
    int rushHourEnd = 1800;
    double rushHourMultiplier = 2.0;
    std::uint32_t randomSeed = 42;

    static Config loadFromFile(const std::filesystem::path& path);
    void validate() const;
    int totalSeats() const noexcept;
    double arrivalRatePerSecond(int simulationTime) const noexcept;
    std::string toString() const;
};

std::string toString(ArrivalPattern pattern);
std::string toString(WindowEfficiency efficiency);
ArrivalPattern parseArrivalPattern(const std::string& value);
WindowEfficiency parseWindowEfficiency(const std::string& value);
std::ostream& operator<<(std::ostream& os, const Config& config);

} // namespace bdss::core
