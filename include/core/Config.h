#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace bdss::core {

enum class ArrivalPattern {
    Steady,
    RushPeaks
};

enum class WindowEfficiency {
    Uniform,
    Variable,
    Custom
};

struct RushPeak {
    int start = 900;      // seconds since simulation starts
    int end = 2400;
    double multiplier = 2.5;
};

struct StudentTypeProfile {
    std::string name = "本科生";
    double ratio = 0.65;
    double diningTimeFactor = 0.85;
    double patienceFactor = 1.0;
};

struct WindowProfile {
    std::string name;
    std::string category;
    double efficiency = 1.0;
};

struct Config {
    // Scale
    int windowCount = 8;
    int tableRows = 10;
    int tableCols = 10;
    int totalSimulationTime = 6600;
    unsigned int randomSeed = 42;

    // Arrival and service, in seconds and people/minute.
    double arrivalRate = 6.0;
    int avgServiceTime = 25;
    double serviceStddev = 8.0;
    int avgDiningTime = 900;
    double diningStddev = 240.0;
    ArrivalPattern arrivalPattern = ArrivalPattern::RushPeaks;
    WindowEfficiency windowEfficiency = WindowEfficiency::Variable;
    std::vector<RushPeak> rushPeaks{{900, 2400, 2.4},
                                    {3000, 4500, 1.8}};

    // Preferences and student profiles.
    bool enableWindowPreferences = true;
    std::vector<std::string> windowCategories{"米饭", "面食", "快餐", "清真", "饮品"};
    std::vector<WindowProfile> windowProfiles{};
    std::vector<StudentTypeProfile> studentTypes{{"本科生", 0.65, 0.85, 1.00},
                                                 {"研究生", 0.25, 1.00, 1.10},
                                                 {"教职工", 0.10, 1.25, 1.25}};
    double queueWeight = 1.0;
    double preferenceBonus = 3.0;
    double efficiencyWeight = 1.0;

    // Patience and reneging.
    bool enablePatience = true;
    double avgPatienceTime = 420.0;
    double patienceStddev = 90.0;
    double queueSwitchProbability = 0.12;

    // Takeaway / packing.
    bool enableTakeaway = true;
    double takeawayRate = 0.18;
    double packingServiceFactor = 1.25;

    // Seats.
    bool enableCleaning = true;
    int cleaningTime = 45;
    bool enableGroupDining = true;
    double groupProbability = 0.22;
    int maxGroupSize = 4;
    bool enableSeatPreference = true;
    double nearWindowWeight = 0.25;
    double groupAdjacencyBonus = 2.5;
    double strangerSpacingPenalty = 0.5;

    void normalize();
    void validate() const;
    static Config loadFromFile(const std::filesystem::path& filePath);
    void saveToFile(const std::filesystem::path& filePath) const;
};

std::string toString(ArrivalPattern pattern);
std::string toString(WindowEfficiency efficiency);
ArrivalPattern arrivalPatternFromString(const std::string& text);
WindowEfficiency windowEfficiencyFromString(const std::string& text);

int parseClockTimeToSeconds(const std::string& text);
std::string formatClockTime(int seconds);

} // namespace bdss::core
