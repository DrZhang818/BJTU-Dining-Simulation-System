#pragma once

#include <expected>
#include <filesystem>
#include <string>

namespace bdss::core {

enum class ArrivalPattern { Steady, RushHour };

enum class WindowEfficiency { Uniform, Variable };

struct Config {
    int windowCount{5};             // 窗口数量
    int tableRows{10};              // 总座位数 - 行数
    int tableCols{10};              // 总座位数 - 列数
    int totalSimulationTime{3600};  // 仿真总时长（单位：秒）

    double arrivalRate{5.0};  // 学生到达频率（人/分钟）
    int avgServiceTime{20};   // 窗口平均打饭耗时（秒）
    int avgDiningTime{900};   // 学生平均就餐时长（秒）

    ArrivalPattern arrivalPattern{ArrivalPattern::Steady};
    WindowEfficiency windowEfficiency{WindowEfficiency::Uniform};

    static std::expected<Config, std::string> loadFromFile(const std::filesystem::path& filePath);
};

}  // namespace bdss::core