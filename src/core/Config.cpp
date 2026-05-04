#include "core/Config.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace bdss::core {
namespace {

std::string readAllText(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return {};
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

double readNumber(const std::string& text, const std::string& key, double defaultValue) {
    const std::string token = "\"" + key + "\"";
    const auto keyPos = text.find(token);
    if (keyPos == std::string::npos) {
        return defaultValue;
    }

    const auto colonPos = text.find(':', keyPos + token.size());
    if (colonPos == std::string::npos) {
        return defaultValue;
    }

    const auto valueStart = text.find_first_not_of(" \t\r\n", colonPos + 1);
    if (valueStart == std::string::npos) {
        return defaultValue;
    }

    try {
        return std::stod(text.substr(valueStart));
    } catch (...) {
        return defaultValue;
    }
}

std::string readString(const std::string& text, const std::string& key, const std::string& defaultValue) {
    const std::string token = "\"" + key + "\"";
    const auto keyPos = text.find(token);
    if (keyPos == std::string::npos) {
        return defaultValue;
    }

    const auto colonPos = text.find(':', keyPos + token.size());
    if (colonPos == std::string::npos) {
        return defaultValue;
    }

    const auto firstQuote = text.find('"', colonPos + 1);
    if (firstQuote == std::string::npos) {
        return defaultValue;
    }

    const auto secondQuote = text.find('"', firstQuote + 1);
    if (secondQuote == std::string::npos) {
        return defaultValue;
    }

    return text.substr(firstQuote + 1, secondQuote - firstQuote - 1);
}

ArrivalPattern parseArrivalPattern(const std::string& value) {
    if (value == "RushHour" || value == "rushHour" || value == "rush_hour") {
        return ArrivalPattern::RushHour;
    }
    return ArrivalPattern::Steady;
}

WindowEfficiency parseWindowEfficiency(const std::string& value) {
    if (value == "Variable" || value == "variable") {
        return WindowEfficiency::Variable;
    }
    return WindowEfficiency::Uniform;
}

} // namespace

Config Config::loadFromFile(const std::filesystem::path& filePath) {
    Config config;

    if (!std::filesystem::exists(filePath)) {
        std::cerr << "[WARN] 配置文件不存在，使用默认配置: " << filePath << '\n';
        return config;
    }

    const std::string text = readAllText(filePath);
    if (text.empty()) {
        std::cerr << "[WARN] 配置文件为空或读取失败，使用默认配置: " << filePath << '\n';
        return config;
    }

    config.windowCount = static_cast<int>(readNumber(text, "windowCount", config.windowCount));
    config.tableRows = static_cast<int>(readNumber(text, "tableRows", config.tableRows));
    config.tableCols = static_cast<int>(readNumber(text, "tableCols", config.tableCols));
    config.totalSimulationTime = static_cast<int>(readNumber(text, "totalSimulationTime", config.totalSimulationTime));

    config.arrivalRate = readNumber(text, "arrivalRate", config.arrivalRate);
    config.avgServiceTime = static_cast<int>(readNumber(text, "avgServiceTime", config.avgServiceTime));
    config.avgDiningTime = static_cast<int>(readNumber(text, "avgDiningTime", config.avgDiningTime));

    config.serviceStddev = readNumber(text, "serviceStddev", config.serviceStddev);
    config.diningStddev = readNumber(text, "diningStddev", config.diningStddev);

    config.rushHourStart = static_cast<int>(readNumber(text, "rushHourStart", config.rushHourStart));
    config.rushHourEnd = static_cast<int>(readNumber(text, "rushHourEnd", config.rushHourEnd));
    config.rushHourMultiplier = readNumber(text, "rushHourMultiplier", config.rushHourMultiplier);
    config.randomSeed = static_cast<unsigned int>(readNumber(text, "randomSeed", config.randomSeed));

    config.arrivalPattern = parseArrivalPattern(readString(text, "arrivalPattern", toString(config.arrivalPattern)));
    config.windowEfficiency = parseWindowEfficiency(readString(text, "windowEfficiency", toString(config.windowEfficiency)));

    return config;
}

std::string toString(ArrivalPattern pattern) {
    switch (pattern) {
    case ArrivalPattern::RushHour:
        return "RushHour";
    case ArrivalPattern::Steady:
    default:
        return "Steady";
    }
}

std::string toString(WindowEfficiency efficiency) {
    switch (efficiency) {
    case WindowEfficiency::Variable:
        return "Variable";
    case WindowEfficiency::Uniform:
    default:
        return "Uniform";
    }
}

} // namespace bdss::core