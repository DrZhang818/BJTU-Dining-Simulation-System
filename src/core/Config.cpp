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
    std::smatch match;
    if (std::regex_search(text, match,
            std::regex("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)"))) {
        std::istringstream(match[1].str()) >> target;
    }
}

void readString(const std::string& text, const std::string& key, std::string& target) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch match;
    if (std::regex_search(text, match, pattern)) {
        target = trim(match[1].str());
    }
}

void readBool(const std::string& text, const std::string& key, bool& target) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*(true|false)");
    std::smatch match;
    if (std::regex_search(text, match, pattern)) {
        target = (match[1].str() == "true");
    }
}

void readStringArray(const std::string& text, const std::string& key, std::vector<std::string>& target) {
    // JSON array: ["a", "b", "c"]
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\[([^\\]]*)\\]");
    std::smatch match;
    if (std::regex_search(text, match, pattern)) {
        target.clear();
        std::string content = match[1].str();
        // Extract quoted strings
        std::regex itemPattern("\\\"([^\\\"]*)\\\"");
        std::sregex_iterator it(content.begin(), content.end(), itemPattern);
        std::sregex_iterator end;
        for (; it != end; ++it) {
            target.push_back(trim((*it)[1].str()));
        }
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

std::string toString(ProfileType type) {
    switch (type) {
    case ProfileType::Undergrad:
        return "Undergrad";
    case ProfileType::Grad:
        return "Grad";
    case ProfileType::Staff:
        return "Staff";
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

ProfileType parseProfileType(const std::string& value) {
    const auto normalized = lower(trim(value));
    if (normalized == "undergrad" || normalized == "undergraduate" || normalized == "本科") {
        return ProfileType::Undergrad;
    }
    if (normalized == "grad" || normalized == "graduate" || normalized == "研究生") {
        return ProfileType::Grad;
    }
    if (normalized == "staff" || normalized == "教职工") {
        return ProfileType::Staff;
    }
    throw std::invalid_argument("unsupported profileType: " + value);
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
    readNumber(text, "randomSeed", config.randomSeed);
    // 多段高峰：JSON array 格式 [{"start":600,"end":2700,"multiplier":2},{"start":3900,"end":5100,"multiplier":1.8}]
    {
        std::smatch m;
        if (std::regex_search(text, m, std::regex("\\\"rushPeaks\\\"\\s*:\\s*\\[([^\\]]*)\\]"))) {
            std::vector<Config::RushPeak> peaks;
            std::string content = m[1].str();
            std::regex peakRe("\\{[^}]*\\}");
            std::sregex_iterator it(content.begin(), content.end(), peakRe);
            for (; it != std::sregex_iterator(); ++it) {
                std::string block = it->str();
                Config::RushPeak peak;
                readNumber(block, "start", peak.start);
                readNumber(block, "end", peak.end);
                readNumber(block, "multiplier", peak.multiplier);
                peaks.push_back(peak);
            }
            if (!peaks.empty()) config.rushPeaks = peaks;
        }
    }

    std::string arrivalPatternText = bdss::core::toString(config.arrivalPattern);
    std::string windowEfficiencyText = bdss::core::toString(config.windowEfficiency);
    readString(text, "arrivalPattern", arrivalPatternText);
    readString(text, "windowEfficiency", windowEfficiencyText);
    config.arrivalPattern = parseArrivalPattern(arrivalPatternText);
    config.windowEfficiency = parseWindowEfficiency(windowEfficiencyText);

    // ---- 偏好系统字段 ----（缺失时保持默认值）
    readBool(text, "enablePreferences", config.enablePreferences);
    readNumber(text, "ratioUndergrad", config.ratioUndergrad);
    readNumber(text, "ratioGrad", config.ratioGrad);
    readNumber(text, "ratioStaff", config.ratioStaff);
    readNumber(text, "undergradDiningFactor", config.undergradDiningFactor);
    readNumber(text, "staffDiningFactor", config.staffDiningFactor);
    readNumber(text, "queueLengthWeight", config.queueLengthWeight);
    readNumber(text, "preferenceBonus", config.preferenceBonus);
    readStringArray(text, "windowCategories", config.windowCategories);

    // ---- Phase 2 字段 ----
    readBool(text, "enablePatience", config.enablePatience);
    readNumber(text, "patienceBase", config.patienceBase);
    readNumber(text, "patienceStddev", config.patienceStddev);
    readBool(text, "patienceAllowSwitch", config.patienceAllowSwitch);
    readBool(text, "enablePacking", config.enablePacking);
    readNumber(text, "packingRatio", config.packingRatio);
    readNumber(text, "packingServiceFactor", config.packingServiceFactor);
    readBool(text, "enableCleaning", config.enableCleaning);
    readNumber(text, "cleaningTime", config.cleaningTime);
    readBool(text, "enableGroupDining", config.enableGroupDining);
    readNumber(text, "groupRatio", config.groupRatio);
    readNumber(text, "groupSizeMin", config.groupSizeMin);
    readNumber(text, "groupSizeMax", config.groupSizeMax);

    // ---- 座位偏好字段 ----
    readBool(text, "enableSeatPreference", config.enableSeatPreference);
    readNumber(text, "windowProximityWeight", config.windowProximityWeight);
    readNumber(text, "groupProximityWeight", config.groupProximityWeight);
    readNumber(text, "isolationWeight", config.isolationWeight);

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
    if (rushPeaks.empty()) {
        throw std::invalid_argument("at least one rushPeak is required");
    }
    for (std::size_t i = 0; i < rushPeaks.size(); ++i) {
        if (rushPeaks[i].start < 0 || rushPeaks[i].end < rushPeaks[i].start) {
            throw std::invalid_argument("rushPeaks[" + std::to_string(i) + "].start/end invalid");
        }
        if (rushPeaks[i].multiplier <= 0.0) {
            throw std::invalid_argument("rushPeaks[" + std::to_string(i) + "].multiplier must be positive");
        }
    }

    // 偏好系统校验
    if (enablePreferences) {
        const auto totalRatio = ratioUndergrad + ratioGrad + ratioStaff;
        if (totalRatio <= 0.0) {
            throw std::invalid_argument("sum of profile ratios must be positive");
        }
        if (undergradDiningFactor <= 0.0 || staffDiningFactor <= 0.0) {
            throw std::invalid_argument("profile dining factors must be positive");
        }
        if (!windowCategories.empty() &&
            static_cast<int>(windowCategories.size()) != windowCount) {
            throw std::invalid_argument("windowCategories size must match windowCount when non-empty");
        }
        if (queueLengthWeight <= 0.0) {
            throw std::invalid_argument("queueLengthWeight must be positive");
        }
    }

    // Phase 2：耐心模型校验
    if (enablePatience) {
        if (patienceBase <= 0.0) {
            throw std::invalid_argument("patienceBase must be positive");
        }
        if (patienceStddev < 0.0) {
            throw std::invalid_argument("patienceStddev must be non-negative");
        }
    }

    // Phase 2：打包模式校验
    if (enablePacking) {
        if (packingRatio < 0.0 || packingRatio > 1.0) {
            throw std::invalid_argument("packingRatio must be in [0, 1]");
        }
        if (packingServiceFactor < 1.0) {
            throw std::invalid_argument("packingServiceFactor must be >= 1.0");
        }
    }

    // Phase 2：清洁模式校验
    if (enableCleaning && cleaningTime < 0) {
        throw std::invalid_argument("cleaningTime must be non-negative");
    }

    // Phase 2：结伴就餐校验
    if (enableGroupDining) {
        if (groupRatio < 0.0 || groupRatio > 1.0) {
            throw std::invalid_argument("groupRatio must be in [0, 1]");
        }
        if (groupSizeMin < 2 || groupSizeMax < groupSizeMin) {
            throw std::invalid_argument("groupSizeMin/groupSizeMax invalid");
        }
    }

    // 座位偏好校验
    if (enableSeatPreference) {
        if (windowProximityWeight < 0.0) {
            throw std::invalid_argument("windowProximityWeight must be non-negative");
        }
        if (groupProximityWeight < 0.0) {
            throw std::invalid_argument("groupProximityWeight must be non-negative");
        }
        if (isolationWeight < 0.0) {
            throw std::invalid_argument("isolationWeight must be non-negative");
        }
    }
}

int Config::totalSeats() const noexcept {
    return tableRows * tableCols;
}

double Config::arrivalRatePerSecond(int simulationTime) const noexcept {
    auto ratePerMinute = arrivalRate;
    if (arrivalPattern == ArrivalPattern::RushHour) {
        for (const auto& peak : rushPeaks) {
            if (simulationTime >= peak.start && simulationTime < peak.end) {
                ratePerMinute *= peak.multiplier;
                break;
            }
        }
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
       << ", rushPeaks=[";
    for (std::size_t i = 0; i < rushPeaks.size(); ++i) {
        if (i > 0) os << ";";
        os << "S" << rushPeaks[i].start << "E" << rushPeaks[i].end << "M" << rushPeaks[i].multiplier;
    }
    os << "]"
       << ", randomSeed=" << randomSeed
       << ", enablePreferences=" << (enablePreferences ? "true" : "false");
    if (enablePreferences) {
        os << ", profiles={U:" << ratioUndergrad << ",G:" << ratioGrad << ",S:" << ratioStaff << "}"
           << ", undergradDiningFactor=" << undergradDiningFactor
           << ", staffDiningFactor=" << staffDiningFactor
           << ", queueLengthWeight=" << queueLengthWeight
           << ", preferenceBonus=" << preferenceBonus;
        if (!windowCategories.empty()) {
            os << ", categories=[";
            for (std::size_t i = 0; i < windowCategories.size(); ++i) {
                if (i > 0) os << ",";
                os << windowCategories[i];
            }
            os << "]";
        }
    }
    os << ", enablePatience=" << (enablePatience ? "true" : "false");
    if (enablePatience) {
        os << ", patienceBase=" << patienceBase
           << ", patienceStddev=" << patienceStddev
           << ", patienceAllowSwitch=" << (patienceAllowSwitch ? "true" : "false");
    }
    os << ", enablePacking=" << (enablePacking ? "true" : "false");
    if (enablePacking) {
        os << ", packingRatio=" << packingRatio
           << ", packingServiceFactor=" << packingServiceFactor;
    }
    os << ", enableCleaning=" << (enableCleaning ? "true" : "false");
    if (enableCleaning) {
        os << ", cleaningTime=" << cleaningTime;
    }
    os << ", enableGroupDining=" << (enableGroupDining ? "true" : "false");
    if (enableGroupDining) {
        os << ", groupRatio=" << groupRatio
           << ", groupSizeMin=" << groupSizeMin
           << ", groupSizeMax=" << groupSizeMax;
    }
    os << ", enableSeatPreference=" << (enableSeatPreference ? "true" : "false");
    if (enableSeatPreference) {
        os << ", windowProxWeight=" << windowProximityWeight
           << ", groupProxWeight=" << groupProximityWeight
           << ", isolationWeight=" << isolationWeight;
    }
    return os.str();
}

std::ostream& operator<<(std::ostream& os, const Config& config) {
    return os << config.toString();
}

} // namespace bdss::core
