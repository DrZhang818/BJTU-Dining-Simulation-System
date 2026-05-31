#include "core/Config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

namespace bdss::core {
namespace {

std::string readAll(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open config file: " + path.string());
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string trim(std::string value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    if (first >= last) {
        return {};
    }
    return std::string(first, last);
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

template <typename T>
void readNumber(const std::string& text, const std::string& key, T& target) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
    std::smatch match;
    if (std::regex_search(text, match, pattern)) {
        std::istringstream stream(match[1].str());
        stream >> target;
    }
}

void readString(const std::string& text, const std::string& key, std::string& target) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch match;
    if (std::regex_search(text, match, pattern)) {
        target = trim(match[1].str());
    }
}

} // namespace

std::string toString(ArrivalPattern pattern) {
    switch (pattern) {
    case ArrivalPattern::Uniform:
        return "Uniform";
    case ArrivalPattern::RushHour:
        return "RushHour";
    }
    return "Unknown";
}

std::string toString(WindowEfficiency efficiency) {
    switch (efficiency) {
    case WindowEfficiency::Equal:
        return "Equal";
    case WindowEfficiency::Variable:
        return "Variable";
    }
    return "Unknown";
}

ArrivalPattern parseArrivalPattern(const std::string& value) {
    const auto normalized = lower(trim(value));
    if (normalized == "uniform" || normalized == "flat" || normalized == "constant") {
        return ArrivalPattern::Uniform;
    }
    if (normalized == "rushhour" || normalized == "rush_hour" || normalized == "rush-hour") {
        return ArrivalPattern::RushHour;
    }
    throw std::invalid_argument("unsupported arrivalPattern: " + value);
}

WindowEfficiency parseWindowEfficiency(const std::string& value) {
    const auto normalized = lower(trim(value));
    if (normalized == "equal" || normalized == "same" || normalized == "constant") {
        return WindowEfficiency::Equal;
    }
    if (normalized == "variable" || normalized == "varied") {
        return WindowEfficiency::Variable;
    }
    throw std::invalid_argument("unsupported windowEfficiency: " + value);
}

Config Config::loadFromFile(const std::filesystem::path& path) {
    Config config;
    const auto text = readAll(path);

    readNumber(text, "windowCount", config.windowCount);
    readNumber(text, "tableRows", config.tableRows);
    readNumber(text, "tableCols", config.tableCols);
    readNumber(text, "totalSimulationTime", config.totalSimulationTime);
    readNumber(text, "arrivalRate", config.arrivalRate);
    readNumber(text, "avgServiceTime", config.avgServiceTime);
    readNumber(text, "avgDiningTime", config.avgDiningTime);
    readNumber(text, "serviceStddev", config.serviceStddev);
    readNumber(text, "diningStddev", config.diningStddev);
    readNumber(text, "rushHourStart", config.rushHourStart);
    readNumber(text, "rushHourEnd", config.rushHourEnd);
    readNumber(text, "rushHourMultiplier", config.rushHourMultiplier);
    readNumber(text, "randomSeed", config.randomSeed);

    std::string arrivalPatternText = bdss::core::toString(config.arrivalPattern);
    std::string windowEfficiencyText = bdss::core::toString(config.windowEfficiency);
    readString(text, "arrivalPattern", arrivalPatternText);
    readString(text, "windowEfficiency", windowEfficiencyText);
    config.arrivalPattern = parseArrivalPattern(arrivalPatternText);
    config.windowEfficiency = parseWindowEfficiency(windowEfficiencyText);

    config.validate();
    return config;
}

void Config::validate() const {
    if (windowCount <= 0) {
        throw std::invalid_argument("windowCount must be positive");
    }
    if (tableRows <= 0 || tableCols <= 0) {
        throw std::invalid_argument("tableRows and tableCols must be positive");
    }
    if (totalSimulationTime <= 0) {
        throw std::invalid_argument("totalSimulationTime must be positive");
    }
    if (arrivalRate < 0.0) {
        throw std::invalid_argument("arrivalRate must be non-negative");
    }
    if (avgServiceTime <= 0 || avgDiningTime <= 0) {
        throw std::invalid_argument("avgServiceTime and avgDiningTime must be positive");
    }
    if (serviceStddev < 0.0 || diningStddev < 0.0) {
        throw std::invalid_argument("serviceStddev and diningStddev must be non-negative");
    }
    if (rushHourStart < 0 || rushHourEnd < rushHourStart) {
        throw std::invalid_argument("rushHourStart/rushHourEnd is invalid");
    }
    if (rushHourMultiplier <= 0.0) {
        throw std::invalid_argument("rushHourMultiplier must be positive");
    }
}

int Config::totalSeats() const noexcept {
    return tableRows * tableCols;
}

double Config::arrivalRatePerSecond(int simulationTime) const noexcept {
    auto ratePerMinute = arrivalRate;
    if (arrivalPattern == ArrivalPattern::RushHour &&
        simulationTime >= rushHourStart &&
        simulationTime < rushHourEnd) {
        ratePerMinute *= rushHourMultiplier;
    }
    return ratePerMinute / 60.0;
}

std::string Config::toString() const {
    std::ostringstream os;
    os << "windowCount=" << windowCount
       << ", tableRows=" << tableRows
       << ", tableCols=" << tableCols
       << ", totalSimulationTime=" << totalSimulationTime
       << ", arrivalRate=" << arrivalRate << " students/min"
       << ", avgServiceTime=" << avgServiceTime
       << ", avgDiningTime=" << avgDiningTime
       << ", serviceStddev=" << serviceStddev
       << ", diningStddev=" << diningStddev
       << ", arrivalPattern=" << bdss::core::toString(arrivalPattern)
       << ", windowEfficiency=" << bdss::core::toString(windowEfficiency)
       << ", rushHourStart=" << rushHourStart
       << ", rushHourEnd=" << rushHourEnd
       << ", rushHourMultiplier=" << rushHourMultiplier
       << ", randomSeed=" << randomSeed;
    return os.str();
}

std::ostream& operator<<(std::ostream& os, const Config& config) {
    return os << config.toString();
}

} // namespace bdss::core
