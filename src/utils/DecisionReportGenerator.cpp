#include "utils/DecisionReportGenerator.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace bdss::utils {
namespace {

std::string oneDecimal(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value;
    return out.str();
}

std::string minutes(double seconds) {
    return oneDecimal(seconds / 60.0) + " 分钟";
}

std::string percent(double value) {
    return oneDecimal(value * 100.0) + "%";
}

std::string yesNo(bool value) {
    return value ? "是" : "否";
}

} // namespace

std::string DecisionReportGenerator::generateMarkdown(
    const bdss::core::Config& config,
    const StatisticsLogger& logger,
    const DecisionReportOptions& options) {
    const auto s = logger.summary();
    const int targetPeople = options.expectedPeople > 0 ? options.expectedPeople : s.generatedStudents;
    const int currentSeats = config.totalSeats;

    const bool windowBottleneck =
        s.averageQueueWait >= 180.0 ||
        s.maxQueueLength >= std::max(20, config.windowCount * 8);
    const bool seatBottleneck =
        s.averageSeatWait >= 120.0 ||
        s.maxWaitingForSeat >= std::max(10, currentSeats / 10) ||
        s.averageSeatUtilization >= 0.85;
    const bool lossRisk = s.dropRate >= 0.05;

    double scale = 1.0;
    if (s.generatedStudents > 0 && targetPeople > 0) {
        scale = static_cast<double>(targetPeople) / static_cast<double>(s.generatedStudents);
    }
    const int suggestedWindows = std::max(config.windowCount, static_cast<int>(std::ceil(config.windowCount * scale)));
    const int suggestedSeats = std::max(currentSeats, static_cast<int>(std::ceil(currentSeats * scale)));

    std::ostringstream md;
    md << "# 食堂就餐仿真 AI 决策报告\n\n";
    md << "> 本报告由 BDSS 仿真结果自动生成，可直接与 CSV 数据、趋势图一起发送给 AI，"
          "用于总结窗口、座位、错峰和打包策略调整方案。\n\n";

    md << "## 1. 场景信息\n\n";
    md << "- 场景名称：" << options.scenarioName << "\n";
    md << "- 预计就餐人数：" << targetPeople << " 人\n";
    md << "- 实际仿真生成人数：" << s.generatedStudents << " 人\n";
    md << "- 仿真时长：" << config.totalSimulationTime << " 秒\n";
    md << "- 当前开放窗口数：" << config.windowCount << " 个\n";
    md << "- 当前座位数：" << currentSeats << " 个\n";
    md << "- 基础到达率：" << oneDecimal(config.arrivalRate) << " 人/分钟\n";
    md << "- 平均服务时间：" << oneDecimal(config.avgServiceTime) << " 秒\n";
    md << "- 平均就餐时间：" << oneDecimal(config.avgDiningTime) << " 秒\n";
    md << "- 外带比例：" << percent(config.takeawayRate) << "\n\n";

    md << "## 2. 核心指标\n\n";
    md << "| 指标 | 结果 |\n";
    md << "|---|---:|\n";
    md << "| 完成就餐人数 | " << s.finishedStudents << " |\n";
    md << "| 中途离开人数 | " << s.droppedStudents << " |\n";
    md << "| 外带人数 | " << s.takeawayStudents << " |\n";
    md << "| 完成率 | " << percent(s.finishRate) << " |\n";
    md << "| 离开率 | " << percent(s.dropRate) << " |\n";
    md << "| 最大排队人数 | " << s.maxQueueLength << " |\n";
    md << "| 排队峰值时间 | 第 " << s.peakQueueTime << " 秒 |\n";
    md << "| 最大等座人数 | " << s.maxWaitingForSeat << " |\n";
    md << "| 等座峰值时间 | 第 " << s.peakSeatWaitTime << " 秒 |\n";
    md << "| 平均排队等待时间 | " << minutes(s.averageQueueWait) << " |\n";
    md << "| 平均等座时间 | " << minutes(s.averageSeatWait) << " |\n";
    md << "| 平均座位利用率 | " << percent(s.averageSeatUtilization) << " |\n\n";

    md << "## 3. 瓶颈判断\n\n";
    md << "| 判断项 | 结论 | 依据 |\n";
    md << "|---|---|---|\n";
    md << "| 窗口服务是否瓶颈 | " << yesNo(windowBottleneck) << " | 平均排队 " << minutes(s.averageQueueWait)
       << "，最大排队 " << s.maxQueueLength << " 人 |\n";
    md << "| 座位资源是否瓶颈 | " << yesNo(seatBottleneck) << " | 平均等座 " << minutes(s.averageSeatWait)
       << "，平均座位利用率 " << percent(s.averageSeatUtilization) << " |\n";
    md << "| 学生流失风险是否偏高 | " << yesNo(lossRisk) << " | 离开率 " << percent(s.dropRate) << " |\n\n";

    md << "## 4. 自动生成的初步调整方案\n\n";
    md << "### 短期方案\n\n";
    if (windowBottleneck) {
        md << "- 高峰期临时增开 1-2 个快速窗口，优先承接出餐速度快、选择频率高的菜品。\n";
        md << "- 设置预打包/快速取餐通道，减少普通窗口服务时间。\n";
        md << "- 将排队长度长期较高的窗口拆分或改成多队列分流。\n";
    } else {
        md << "- 当前窗口压力可控，短期重点应放在高峰前提示和队列引导。\n";
    }
    if (seatBottleneck) {
        md << "- 高峰时段提高外带引导比例，并开放临时座位或备用就餐区。\n";
        md << "- 缩短清洁周转时间，安排专人处理翻台区域。\n";
    }
    if (lossRisk) {
        md << "- 对等待时间超过阈值的时段做重点干预，降低学生放弃就餐概率。\n";
    }

    md << "\n### 中期方案\n\n";
    md << "- 用本次 CSV 中的 `total_queue_length`、`waiting_for_seat_count`、`seat_utilization` 找出峰值区间，"
          "针对峰值前后 10-15 分钟做错峰引导。\n";
    md << "- 将窗口按照服务速度和受欢迎程度重新分组，减少热门窗口过载、冷门窗口空闲的情况。\n";
    md << "- 对外带比例、窗口数、座位数分别做多轮仿真，比较完成率和平均等待时间。\n";

    md << "\n### 长期方案\n\n";
    md << "- 建立常态化仿真评估流程：每次调整参数后重新运行仿真，比较完成率、离开率和峰值拥堵。\n";
    md << "- 结合真实刷卡/门禁/订单数据校准到达率、服务时间、就餐时间和外带比例。\n";

    md << "\n## 5. 按预计人数放大后的参数建议\n\n";
    md << "| 参数 | 当前值 | 建议参考值 | 说明 |\n";
    md << "|---|---:|---:|---|\n";
    md << "| 开放窗口数 | " << config.windowCount << " | " << suggestedWindows << " | 按目标人数比例放大，需二次仿真验证 |\n";
    md << "| 座位数 | " << currentSeats << " | " << suggestedSeats << " | 若座位瓶颈明显，应优先验证该值 |\n";
    md << "| 外带比例 | " << percent(config.takeawayRate) << " | "
       << percent(std::min(0.60, config.takeawayRate + (seatBottleneck ? 0.10 : 0.05)))
       << " | 用于缓解座位压力，不宜无限提高 |\n\n";

    md << "## 6. 建议配套发送给 AI 的文件\n\n";
    md << "- `simulation_log.csv`：原始仿真时间序列数据。\n";
    md << "- 排队人数趋势图：对应 `total_queue_length`。\n";
    md << "- 等座人数趋势图：对应 `waiting_for_seat_count`。\n";
    md << "- 座位利用率趋势图：对应 `seat_utilization`。\n";
    md << "- 窗口队列/服务量对比图：对应 `window_queue_lengths`、`window_served_counts`。\n\n";

    if (options.includeAiPrompt) {
        md << "## 7. 可直接复制给 AI 的提示词\n\n";
        md << "```text\n";
        md << "你是一名高校食堂运营优化顾问。请基于我提供的仿真报告、CSV 数据和趋势图，"
              "输出一份面向食堂决策者的调整方案。\n\n";
        md << "请完成以下任务：\n";
        md << "1. 判断主要瓶颈是窗口服务能力、座位资源、到达高峰，还是学生等待流失。\n";
        md << "2. 用报告中的数据说明判断依据，不要编造没有提供的数据。\n";
        md << "3. 输出短期、中期、长期三类调整建议。\n";
        md << "4. 每条建议都说明：依据、预期效果、潜在风险。\n";
        md << "5. 最后给出下一轮仿真应调整的参数，例如窗口数、座位数、外带比例、到达率、服务时间。\n";
        md << "```\n";
    }

    return md.str();
}

void DecisionReportGenerator::exportMarkdown(
    const std::filesystem::path& filePath,
    const bdss::core::Config& config,
    const StatisticsLogger& logger,
    const DecisionReportOptions& options) {
    std::ofstream output(filePath);
    if (!output) {
        throw std::runtime_error("Cannot write decision report: " + filePath.string());
    }
    output << generateMarkdown(config, logger, options);
}

} // namespace bdss::utils
