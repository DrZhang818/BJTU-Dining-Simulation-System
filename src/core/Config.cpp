#include "core/Config.h"

#include "utils/SimpleJson.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace bdss::core {
namespace {
using bdss::utils::JsonValue;

const JsonValue* member(const JsonValue& object, const std::string& key) {
    return object.find(key);
}

int getInt(const JsonValue& object, const std::string& key, int fallback) {
    const JsonValue* value = member(object, key);
    return value ? value->asInt(fallback) : fallback;
}

double getDouble(const JsonValue& object, const std::string& key, double fallback) {
    const JsonValue* value = member(object, key);
    return value ? value->asNumber(fallback) : fallback;
}

bool getBool(const JsonValue& object, const std::string& key, bool fallback) {
    const JsonValue* value = member(object, key);
    return value ? value->asBool(fallback) : fallback;
}

std::string getString(const JsonValue& object, const std::string& key, std::string fallback) {
    const JsonValue* value = member(object, key);
    return value ? value->asStringOr(std::move(fallback)) : std::move(fallback);
}

std::vector<std::string> getStringArray(const JsonValue& object,
                                        const std::string& key,
                                        std::vector<std::string> fallback) {
    const JsonValue* value = member(object, key);
    if (!value || !value->isArray()) {
        return fallback;
    }
    std::vector<std::string> result;
    for (const JsonValue& item : value->asArray()) {
        if (item.isString()) {
            result.push_back(item.asString());
        }
    }
    return result.empty() ? std::move(fallback) : result;
}

JsonValue::Array toArray(const std::vector<std::string>& values) {
    JsonValue::Array array;
    for (const auto& value : values) {
        array.emplace_back(value);
    }
    return array;
}

JsonValue::Array rushPeakArray(const std::vector<RushPeak>& peaks) {
    JsonValue::Array array;
    for (const auto& peak : peaks) {
        JsonValue::Object obj;
        obj["start"] = formatClockTime(peak.start);
        obj["end"] = formatClockTime(peak.end);
        obj["multiplier"] = peak.multiplier;
        array.emplace_back(obj);
    }
    return array;
}

JsonValue::Array windowProfileArray(const std::vector<WindowProfile>& profiles) {
    JsonValue::Array array;
    for (const auto& profile : profiles) {
        JsonValue::Object obj;
        obj["name"] = profile.name;
        obj["category"] = profile.category;
        obj["efficiency"] = profile.efficiency;
        array.emplace_back(obj);
    }
    return array;
}

JsonValue::Array studentTypeArray(const std::vector<StudentTypeProfile>& profiles) {
    JsonValue::Array array;
    for (const auto& profile : profiles) {
        JsonValue::Object obj;
        obj["name"] = profile.name;
        obj["ratio"] = profile.ratio;
        obj["diningTimeFactor"] = profile.diningTimeFactor;
        obj["patienceFactor"] = profile.patienceFactor;
        array.emplace_back(obj);
    }
    return array;
}

int secondsFromJsonValue(const JsonValue& value, int fallback) {
    if (value.isString()) {
        return parseClockTimeToSeconds(value.asString());
    }
    if (value.isNumber()) {
        return value.asInt(fallback);
    }
    return fallback;
}

std::vector<RushPeak> parseRushPeaks(const JsonValue& root, std::vector<RushPeak> fallback) {
    const JsonValue* value = member(root, "rushPeaks");
    if (!value || !value->isArray()) {
        if (root.contains("rushHourStart") || root.contains("rushHourEnd") || root.contains("rushHourMultiplier")) {
            RushPeak peak;
            peak.start = getInt(root, "rushHourStart", peak.start);
            peak.end = getInt(root, "rushHourEnd", peak.end);
            peak.multiplier = getDouble(root, "rushHourMultiplier", peak.multiplier);
            return {peak};
        }
        return fallback;
    }

    std::vector<RushPeak> result;
    for (const auto& item : value->asArray()) {
        if (!item.isObject()) {
            continue;
        }
        RushPeak peak;
        if (const JsonValue* start = item.find("start")) {
            peak.start = secondsFromJsonValue(*start, peak.start);
        }
        if (const JsonValue* end = item.find("end")) {
            peak.end = secondsFromJsonValue(*end, peak.end);
        }
        peak.multiplier = getDouble(item, "multiplier", peak.multiplier);
        result.push_back(peak);
    }
    return result.empty() ? std::move(fallback) : result;
}

std::vector<WindowProfile> parseWindowProfiles(const JsonValue& root) {
    std::vector<WindowProfile> result;
    const JsonValue* value = member(root, "windowProfiles");
    if (!value || !value->isArray()) {
        return result;
    }
    for (const auto& item : value->asArray()) {
        if (!item.isObject()) {
            continue;
        }
        WindowProfile profile;
        profile.name = getString(item, "name", profile.name);
        profile.category = getString(item, "category", profile.category);
        profile.efficiency = getDouble(item, "efficiency", profile.efficiency);
        result.push_back(profile);
    }
    return result;
}

std::vector<StudentTypeProfile> parseStudentTypes(const JsonValue& root,
                                                  std::vector<StudentTypeProfile> fallback) {
    const JsonValue* value = member(root, "studentTypes");
    if (!value || !value->isArray()) {
        return fallback;
    }
    std::vector<StudentTypeProfile> result;
    for (const auto& item : value->asArray()) {
        if (!item.isObject()) {
            continue;
        }
        StudentTypeProfile profile;
        profile.name = getString(item, "name", profile.name);
        profile.ratio = getDouble(item, "ratio", profile.ratio);
        profile.diningTimeFactor = getDouble(item, "diningTimeFactor", profile.diningTimeFactor);
        profile.patienceFactor = getDouble(item, "patienceFactor", profile.patienceFactor);
        result.push_back(profile);
    }
    return result.empty() ? std::move(fallback) : result;
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::invalid_argument("Invalid configuration: " + message);
    }
}

} // namespace

std::string toString(ArrivalPattern pattern) {
    switch (pattern) {
        case ArrivalPattern::Steady: return "Steady";
        case ArrivalPattern::RushPeaks: return "RushPeaks";
    }
    return "Steady";
}

std::string toString(WindowEfficiency efficiency) {
    switch (efficiency) {
        case WindowEfficiency::Uniform: return "Uniform";
        case WindowEfficiency::Variable: return "Variable";
        case WindowEfficiency::Custom: return "Custom";
    }
    return "Uniform";
}

ArrivalPattern arrivalPatternFromString(const std::string& text) {
    if (text == "RushPeaks" || text == "RushHour" || text == "rush") {
        return ArrivalPattern::RushPeaks;
    }
    return ArrivalPattern::Steady;
}

WindowEfficiency windowEfficiencyFromString(const std::string& text) {
    if (text == "Variable" || text == "variable") {
        return WindowEfficiency::Variable;
    }
    if (text == "Custom" || text == "custom") {
        return WindowEfficiency::Custom;
    }
    return WindowEfficiency::Uniform;
}

int parseClockTimeToSeconds(const std::string& text) {
    int h = 0;
    int m = 0;
    int s = 0;
    char sep1 = '\0';
    char sep2 = '\0';
    std::istringstream iss(text);
    if (!(iss >> h >> sep1 >> m) || sep1 != ':') {
        return 0;
    }
    if (iss >> sep2 >> s) {
        if (sep2 != ':') {
            s = 0;
        }
    }
    h = std::clamp(h, 0, 23);
    m = std::clamp(m, 0, 59);
    s = std::clamp(s, 0, 59);
    return h * 3600 + m * 60 + s;
}

std::string formatClockTime(int seconds) {
    seconds = ((seconds % (24 * 3600)) + 24 * 3600) % (24 * 3600);
    const int h = seconds / 3600;
    const int m = (seconds % 3600) / 60;
    const int s = seconds % 60;
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << h << ':'
        << std::setw(2) << std::setfill('0') << m;
    if (s != 0) {
        oss << ':' << std::setw(2) << std::setfill('0') << s;
    }
    return oss.str();
}

void Config::normalize() {
    if (windowCategories.empty()) {
        windowCategories = {"综合窗口"};
    }
    if (rushPeaks.empty()) {
        rushPeaks = {{900, 2400, 2.4}};
    }
    if (studentTypes.empty()) {
        studentTypes = {{"本科生", 0.65, 0.85, 1.00},
                        {"研究生", 0.25, 1.00, 1.10},
                        {"教职工", 0.10, 1.25, 1.25}};
    }
    double ratioSum = 0.0;
    for (const auto& type : studentTypes) {
        ratioSum += std::max(0.0, type.ratio);
    }
    if (ratioSum <= 0.0) {
        studentTypes = {{"本科生", 1.0, 1.0, 1.0}};
    }

    if (windowCount > 0) {
        if (windowProfiles.size() > static_cast<std::size_t>(windowCount)) {
            windowProfiles.resize(static_cast<std::size_t>(windowCount));
        }
        while (windowProfiles.size() < static_cast<std::size_t>(windowCount)) {
            windowProfiles.push_back(WindowProfile{});
        }
        for (int i = 0; i < windowCount; ++i) {
            auto& profile = windowProfiles[static_cast<std::size_t>(i)];
            if (profile.name.empty()) {
                profile.name = "窗口 " + std::to_string(i + 1);
            }
            if (profile.category.empty()) {
                profile.category = windowCategories[static_cast<std::size_t>(i) % windowCategories.size()];
            }
            if (profile.efficiency <= 0.0) {
                profile.efficiency = 1.0;
            }
        }
    }
}

void Config::validate() const {
    require(windowCount > 0 && windowCount <= 100, "windowCount must be between 1 and 100");
    require(tableRows > 0 && tableRows <= 80, "tableRows must be between 1 and 80");
    require(tableCols > 0 && tableCols <= 80, "tableCols must be between 1 and 80");
    require(totalSimulationTime > 0 && totalSimulationTime <= 24 * 3600, "totalSimulationTime must be within one day");
    require(arrivalRate >= 0.0, "arrivalRate must be non-negative");
    require(avgServiceTime > 0, "avgServiceTime must be positive");
    require(serviceStddev >= 0.0, "serviceStddev must be non-negative");
    require(avgDiningTime > 0, "avgDiningTime must be positive");
    require(diningStddev >= 0.0, "diningStddev must be non-negative");
    require(!windowCategories.empty(), "windowCategories must not be empty");
    require(queueWeight >= 0.0, "queueWeight must be non-negative");
    require(efficiencyWeight >= 0.0, "efficiencyWeight must be non-negative");
    require(avgPatienceTime > 0.0, "avgPatienceTime must be positive");
    require(patienceStddev >= 0.0, "patienceStddev must be non-negative");
    require(queueSwitchProbability >= 0.0 && queueSwitchProbability <= 1.0, "queueSwitchProbability must be in [0, 1]");
    require(takeawayRate >= 0.0 && takeawayRate <= 1.0, "takeawayRate must be in [0, 1]");
    require(packingServiceFactor >= 1.0, "packingServiceFactor must be at least 1.0");
    require(cleaningTime >= 0, "cleaningTime must be non-negative");
    require(groupProbability >= 0.0 && groupProbability <= 1.0, "groupProbability must be in [0, 1]");
    require(maxGroupSize >= 1 && maxGroupSize <= 12, "maxGroupSize must be between 1 and 12");
    require(nearWindowWeight >= 0.0, "nearWindowWeight must be non-negative");
    require(groupAdjacencyBonus >= 0.0, "groupAdjacencyBonus must be non-negative");
    require(strangerSpacingPenalty >= 0.0, "strangerSpacingPenalty must be non-negative");

    for (const auto& peak : rushPeaks) {
        require(peak.start >= 0 && peak.end >= 0 && peak.start < 24 * 3600 && peak.end <= 24 * 3600,
                "rush peak times must be within one day");
        require(peak.start < peak.end, "rush peak start must be earlier than end");
        require(peak.multiplier >= 0.0, "rush peak multiplier must be non-negative");
    }

    double ratioSum = 0.0;
    for (const auto& profile : studentTypes) {
        require(!profile.name.empty(), "student type name must not be empty");
        require(profile.ratio >= 0.0, "student type ratio must be non-negative");
        require(profile.diningTimeFactor > 0.0, "student diningTimeFactor must be positive");
        require(profile.patienceFactor > 0.0, "student patienceFactor must be positive");
        ratioSum += profile.ratio;
    }
    require(!studentTypes.empty() && ratioSum > 0.0, "studentTypes must contain positive ratios");

    for (const auto& profile : windowProfiles) {
        require(profile.efficiency > 0.0, "custom window efficiency must be positive");
    }
}

Config Config::loadFromFile(const std::filesystem::path& filePath) {
    std::ifstream input(filePath);
    if (!input) {
        throw std::runtime_error("Cannot open config file: " + filePath.string());
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    JsonValue root = JsonValue::parse(buffer.str());
    if (!root.isObject()) {
        throw std::runtime_error("Config file root must be a JSON object: " + filePath.string());
    }

    Config config;
    config.windowCount = getInt(root, "windowCount", config.windowCount);
    config.tableRows = getInt(root, "tableRows", config.tableRows);
    config.tableCols = getInt(root, "tableCols", config.tableCols);
    config.totalSimulationTime = getInt(root, "totalSimulationTime", config.totalSimulationTime);
    config.randomSeed = static_cast<unsigned int>(getInt(root, "randomSeed", static_cast<int>(config.randomSeed)));

    config.arrivalRate = getDouble(root, "arrivalRate", config.arrivalRate);
    config.avgServiceTime = getInt(root, "avgServiceTime", config.avgServiceTime);
    config.serviceStddev = getDouble(root, "serviceStddev", config.serviceStddev);
    config.avgDiningTime = getInt(root, "avgDiningTime", config.avgDiningTime);
    config.diningStddev = getDouble(root, "diningStddev", config.diningStddev);
    config.arrivalPattern = arrivalPatternFromString(getString(root, "arrivalPattern", toString(config.arrivalPattern)));
    config.windowEfficiency = windowEfficiencyFromString(getString(root, "windowEfficiency", toString(config.windowEfficiency)));
    config.rushPeaks = parseRushPeaks(root, config.rushPeaks);

    config.enableWindowPreferences = getBool(root, "enableWindowPreferences", config.enableWindowPreferences);
    config.enableWindowPreferences = getBool(root, "enablePreferences", config.enableWindowPreferences);
    config.windowCategories = getStringArray(root, "windowCategories", config.windowCategories);
    config.windowProfiles = parseWindowProfiles(root);
    config.studentTypes = parseStudentTypes(root, config.studentTypes);
    if (!root.contains("studentTypes") &&
        (root.contains("ratioUndergrad") || root.contains("ratioGrad") || root.contains("ratioStaff"))) {
        config.studentTypes = {
            {"本科生", getDouble(root, "ratioUndergrad", 0.65), getDouble(root, "undergradDiningFactor", 0.85), 1.00},
            {"研究生", getDouble(root, "ratioGrad", 0.25), getDouble(root, "gradDiningFactor", 1.00), 1.10},
            {"教职工", getDouble(root, "ratioStaff", 0.10), getDouble(root, "staffDiningFactor", 1.25), 1.25}
        };
    }
    config.queueWeight = getDouble(root, "queueWeight", config.queueWeight);
    config.queueWeight = getDouble(root, "queueLengthWeight", config.queueWeight);
    config.preferenceBonus = getDouble(root, "preferenceBonus", config.preferenceBonus);
    config.efficiencyWeight = getDouble(root, "efficiencyWeight", config.efficiencyWeight);

    config.enablePatience = getBool(root, "enablePatience", config.enablePatience);
    config.avgPatienceTime = getDouble(root, "avgPatienceTime", config.avgPatienceTime);
    config.avgPatienceTime = getDouble(root, "patienceBase", config.avgPatienceTime);
    config.patienceStddev = getDouble(root, "patienceStddev", config.patienceStddev);
    config.queueSwitchProbability = getDouble(root, "queueSwitchProbability", config.queueSwitchProbability);
    if (const JsonValue* allowSwitch = member(root, "patienceAllowSwitch"); allowSwitch && allowSwitch->isBool() && !allowSwitch->asBool()) {
        config.queueSwitchProbability = 0.0;
    }

    config.enableTakeaway = getBool(root, "enableTakeaway", config.enableTakeaway);
    config.enableTakeaway = getBool(root, "enablePacking", config.enableTakeaway);
    config.takeawayRate = getDouble(root, "takeawayRate", config.takeawayRate);
    config.takeawayRate = getDouble(root, "packingRatio", config.takeawayRate);
    config.packingServiceFactor = getDouble(root, "packingServiceFactor", config.packingServiceFactor);

    config.enableCleaning = getBool(root, "enableCleaning", config.enableCleaning);
    config.cleaningTime = getInt(root, "cleaningTime", config.cleaningTime);
    config.enableGroupDining = getBool(root, "enableGroupDining", config.enableGroupDining);
    config.groupProbability = getDouble(root, "groupProbability", config.groupProbability);
    config.groupProbability = getDouble(root, "groupRatio", config.groupProbability);
    config.maxGroupSize = getInt(root, "maxGroupSize", config.maxGroupSize);
    config.maxGroupSize = getInt(root, "groupSizeMax", config.maxGroupSize);
    config.enableSeatPreference = getBool(root, "enableSeatPreference", config.enableSeatPreference);
    config.nearWindowWeight = getDouble(root, "nearWindowWeight", config.nearWindowWeight);
    config.nearWindowWeight = getDouble(root, "windowProximityWeight", config.nearWindowWeight);
    config.groupAdjacencyBonus = getDouble(root, "groupAdjacencyBonus", config.groupAdjacencyBonus);
    config.groupAdjacencyBonus = getDouble(root, "groupProximityWeight", config.groupAdjacencyBonus);
    config.strangerSpacingPenalty = getDouble(root, "strangerSpacingPenalty", config.strangerSpacingPenalty);
    config.strangerSpacingPenalty = getDouble(root, "isolationWeight", config.strangerSpacingPenalty);

    config.normalize();
    config.validate();
    return config;
}

void Config::saveToFile(const std::filesystem::path& filePath) const {
    Config copy = *this;
    copy.normalize();
    copy.validate();

    JsonValue::Object root;
    root["windowCount"] = copy.windowCount;
    root["tableRows"] = copy.tableRows;
    root["tableCols"] = copy.tableCols;
    root["totalSimulationTime"] = copy.totalSimulationTime;
    root["randomSeed"] = static_cast<int>(copy.randomSeed);
    root["arrivalRate"] = copy.arrivalRate;
    root["avgServiceTime"] = copy.avgServiceTime;
    root["serviceStddev"] = copy.serviceStddev;
    root["avgDiningTime"] = copy.avgDiningTime;
    root["diningStddev"] = copy.diningStddev;
    root["arrivalPattern"] = toString(copy.arrivalPattern);
    root["windowEfficiency"] = toString(copy.windowEfficiency);
    root["rushPeaks"] = rushPeakArray(copy.rushPeaks);
    root["enableWindowPreferences"] = copy.enableWindowPreferences;
    root["windowCategories"] = toArray(copy.windowCategories);
    root["windowProfiles"] = windowProfileArray(copy.windowProfiles);
    root["studentTypes"] = studentTypeArray(copy.studentTypes);
    root["queueWeight"] = copy.queueWeight;
    root["preferenceBonus"] = copy.preferenceBonus;
    root["efficiencyWeight"] = copy.efficiencyWeight;
    root["enablePatience"] = copy.enablePatience;
    root["avgPatienceTime"] = copy.avgPatienceTime;
    root["patienceStddev"] = copy.patienceStddev;
    root["queueSwitchProbability"] = copy.queueSwitchProbability;
    root["enableTakeaway"] = copy.enableTakeaway;
    root["takeawayRate"] = copy.takeawayRate;
    root["packingServiceFactor"] = copy.packingServiceFactor;
    root["enableCleaning"] = copy.enableCleaning;
    root["cleaningTime"] = copy.cleaningTime;
    root["enableGroupDining"] = copy.enableGroupDining;
    root["groupProbability"] = copy.groupProbability;
    root["maxGroupSize"] = copy.maxGroupSize;
    root["enableSeatPreference"] = copy.enableSeatPreference;
    root["nearWindowWeight"] = copy.nearWindowWeight;
    root["groupAdjacencyBonus"] = copy.groupAdjacencyBonus;
    root["strangerSpacingPenalty"] = copy.strangerSpacingPenalty;

    std::ofstream output(filePath);
    if (!output) {
        throw std::runtime_error("Cannot write config file: " + filePath.string());
    }
    output << JsonValue(root).dump(2) << '\n';
}

} // namespace bdss::core
