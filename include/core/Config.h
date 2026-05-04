#pragma once

#include <filesystem>
#include <string>

namespace bdss::core {

enum class ArrivalPattern {
    Steady,
    RushHour
};

enum class WindowEfficiency {
    Uniform,
    Variable
};

struct Config {
    int windowCount = 5;               // 窗口数量
    int tableRows = 10;                // 餐桌矩阵行数
    int tableCols = 10;                // 餐桌矩阵列数
    int totalSimulationTime = 3600;    // 学生持续到达的仿真时长，单位：秒

    double arrivalRate = 5.0;          // 学生平均到达频率，单位：人/分钟
    int avgServiceTime = 20;           // 窗口平均打饭耗时，单位：秒
    int avgDiningTime = 900;           // 学生平均就餐时长，单位：秒

    double serviceStddev = 5.0;        // 打饭耗时标准差
    double diningStddev = 180.0;       // 就餐时长标准差

    ArrivalPattern arrivalPattern = ArrivalPattern::Steady;
    WindowEfficiency windowEfficiency = WindowEfficiency::Uniform;

    int rushHourStart = 900;           // 高峰开始时间，单位：秒
    int rushHourEnd = 1800;            // 高峰结束时间，单位：秒
    double rushHourMultiplier = 2.0;   // 高峰到达率倍率

    unsigned int randomSeed = 42;      // 固定随机种子，便于测试复现

    static Config loadFromFile(const std::filesystem::path& filePath);
};

std::string toString(ArrivalPattern pattern);
std::string toString(WindowEfficiency efficiency);

} // namespace bdss::core