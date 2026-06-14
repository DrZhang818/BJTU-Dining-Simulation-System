#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace bdss::core {

struct RushPeak {
    int start = 0;
    int end = 0;
    double multiplier = 1.0;
};

struct Config {
    int windowCount = 10;
    int totalSeats = 320;
    int totalSimulationTime = 3600;
    double arrivalRate = 7.5;       // people per minute
    double avgServiceTime = 55.0;   // seconds
    double avgDiningTime = 900.0;   // seconds
    double avgCleaningTime = 45.0;  // seconds
    double takeawayRate = 0.18;
    int patienceQueueSeconds = 900;
    int patienceSeatSeconds = 600;
    unsigned int randomSeed = 42;
    std::vector<RushPeak> rushPeaks{{900, 2400, 2.4}};

    static Config loadFromFile(const std::filesystem::path& path);
    void normalize();
    void validate() const;
    double arrivalRateAt(int second) const;
};

} // namespace bdss::core
