#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace bdss::core {

enum class ArrivalPattern {
    Uniform,
    RushHour
};

enum class WindowEfficiency {
    Equal,
    Variable
};

// 就餐者画像类型（A：偏好系统）
enum class ProfileType {
    Undergrad,  // 本科生：赶时间、吃得快
    Grad,       // 研究生
    Staff       // 教职工：偏悠闲、吃得慢
};

std::string toString(ProfileType type);
ProfileType parseProfileType(const std::string& value);

struct Config {
    // 8 个窗口、100 个座位，对应中等食堂规模
    int windowCount = 8;
    int tableRows = 10;
    int tableCols = 10;
    int totalSimulationTime = 6600;  // 11:00-12:50，110 分钟

    double arrivalRate = 6.0;        // 基础 6 人/分钟

    int avgServiceTime = 25;         // 平均打饭 25 秒（含打包影响）
    int avgDiningTime = 900;         // 平均就餐 15 分钟
    double serviceStddev = 8.0;
    double diningStddev = 240.0;

    ArrivalPattern arrivalPattern = ArrivalPattern::RushHour;
    WindowEfficiency windowEfficiency = WindowEfficiency::Variable;

    // 高峰段：12:00-12:15 最高峰，12:20-12:40 次高峰，相隔约 5 分钟
    struct RushPeak {
        int start = 3600;           // 12:00
        int end = 4500;             // 12:15
        double multiplier = 3.0;
    };
    std::vector<RushPeak> rushPeaks = {
        {3600, 4500, 2.5},          // 12:00-12:15 最高峰倍率 2.5
        {4800, 6000, 1.8}           // 12:20-12:40 次高峰倍率 1.8
    };
    std::uint32_t randomSeed = 42;

    // ---- 偏好系统（A）----
    bool enablePreferences = true;
    double ratioUndergrad = 0.65;
    double ratioGrad      = 0.25;
    double ratioStaff     = 0.10;
    double undergradDiningFactor = 0.80;
    double staffDiningFactor     = 1.35;

    // ---- 窗口品类偏好（B）----
    std::vector<std::string> windowCategories = {"麻辣烫", "米饭套餐", "米线", "清真窗口", "西餐简餐", "面食", "韩式料理", "轻食沙拉"};
    double queueLengthWeight = 1.0;
    double preferenceBonus   = 3.0;

    // ---- Phase 2：排队耐心模型（C）----
    bool enablePatience = true;
    double patienceBase = 150.0;
    double patienceStddev = 40.0;
    bool patienceAllowSwitch = true;

    // ---- Phase 2：打包/堂食模式（D）----
    bool enablePacking = true;
    double packingRatio = 0.30;
    double packingServiceFactor = 1.5;

    // ---- Phase 2：座位翻台/清洁（F）----
    bool enableCleaning = true;
    int cleaningTime = 10;

    // ---- Phase 2：结伴就餐 + 相邻座位（E）----
    bool enableGroupDining = true;
    double groupRatio = 0.20;
    int groupSizeMin = 2;
    int groupSizeMax = 4;

    // ---- Phase 2：座位偏好（G）----
    bool enableSeatPreference = true;
    double windowProximityWeight = 1.5;
    double groupProximityWeight  = 2.0;
    double isolationWeight       = 3.0;

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
