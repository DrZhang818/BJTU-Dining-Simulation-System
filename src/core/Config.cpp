#include "core/Config.h"

#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace bdss::core {
namespace {

std::string readAll(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Cannot open config file: " + path.string());
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

template <typename T>
void readNumber(const std::string& text, const std::string& key, T& target) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
    std::smatch match;
    if (std::regex_search(text, match, pattern)) {
        if constexpr (std::is_integral<T>::value) {
            target = static_cast<T>(std::stoll(match[1].str()));
        } else {
            target = static_cast<T>(std::stod(match[1].str()));
        }
    }
}

std::vector<RushPeak> readRushPeaks(const std::string& text) {
    std::vector<RushPeak> peaks;
    const std::regex objectPattern(
        R"(\{\s*"start"\s*:\s*(-?[0-9]+)\s*,\s*"end"\s*:\s*(-?[0-9]+)\s*,\s*"multiplier"\s*:\s*([0-9]+(?:\.[0-9]+)?)\s*\})");
    for (auto it = std::sregex_iterator(text.begin(), text.end(), objectPattern);
         it != std::sregex_iterator(); ++it) {
        RushPeak peak;
        peak.start = std::stoi((*it)[1].str());
        peak.end = std::stoi((*it)[2].str());
        peak.multiplier = std::stod((*it)[3].str());
        peaks.push_back(peak);
    }
    return peaks;
}

} // namespace

Config Config::loadFromFile(const std::filesystem::path& path) {
    Config config;
    const auto text = readAll(path);

    readNumber(text, "windowCount", config.windowCount);
    readNumber(text, "totalSeats", config.totalSeats);
    readNumber(text, "seatCount", config.totalSeats);
    readNumber(text, "totalSimulationTime", config.totalSimulationTime);
    readNumber(text, "duration", config.totalSimulationTime);
    readNumber(text, "arrivalRate", config.arrivalRate);
    readNumber(text, "avgServiceTime", config.avgServiceTime);
    readNumber(text, "avgDiningTime", config.avgDiningTime);
    readNumber(text, "avgCleaningTime", config.avgCleaningTime);
    readNumber(text, "takeawayRate", config.takeawayRate);
    readNumber(text, "patienceQueueSeconds", config.patienceQueueSeconds);
    readNumber(text, "patienceSeatSeconds", config.patienceSeatSeconds);
    readNumber(text, "randomSeed", config.randomSeed);

    auto peaks = readRushPeaks(text);
    if (!peaks.empty()) {
        config.rushPeaks = std::move(peaks);
    }

    config.normalize();
    config.validate();
    return config;
}

void Config::normalize() {
    windowCount = std::max(1, windowCount);
    totalSeats = std::max(1, totalSeats);
    totalSimulationTime = std::max(60, totalSimulationTime);
    arrivalRate = std::max(0.0, arrivalRate);
    avgServiceTime = std::max(1.0, avgServiceTime);
    avgDiningTime = std::max(1.0, avgDiningTime);
    avgCleaningTime = std::max(0.0, avgCleaningTime);
    takeawayRate = std::clamp(takeawayRate, 0.0, 1.0);
    patienceQueueSeconds = std::max(1, patienceQueueSeconds);
    patienceSeatSeconds = std::max(1, patienceSeatSeconds);

    for (auto& peak : rushPeaks) {
        peak.start = std::max(0, peak.start);
        peak.end = std::max(peak.start, peak.end);
        peak.multiplier = std::max(0.0, peak.multiplier);
    }
}

void Config::validate() const {
    if (windowCount <= 0) {
        throw std::invalid_argument("windowCount must be positive");
    }
    if (totalSeats <= 0) {
        throw std::invalid_argument("totalSeats must be positive");
    }
    if (totalSimulationTime <= 0) {
        throw std::invalid_argument("totalSimulationTime must be positive");
    }
    if (arrivalRate < 0.0) {
        throw std::invalid_argument("arrivalRate must not be negative");
    }
}

double Config::arrivalRateAt(int second) const {
    double multiplier = 1.0;
    for (const auto& peak : rushPeaks) {
        if (second >= peak.start && second < peak.end) {
            multiplier = std::max(multiplier, peak.multiplier);
        }
    }
    return arrivalRate * multiplier;
}

} // namespace bdss::core
