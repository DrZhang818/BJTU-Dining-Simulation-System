# BJTU Dining Simulation System · AI Report Edition

这是一个可编译的 C++17 食堂就餐仿真源码包，包含“根据预计就餐人数导出 AI 决策报告”的功能。

## 功能

- 离散时间食堂仿真：学生到达、窗口排队、服务、等座、就餐、清洁、外带和流失。
- CSV 输出：逐秒记录排队、等座、座位利用率和窗口服务量。
- AI 决策报告：根据仿真结果和预计人数生成 Markdown 报告，方便直接发给 AI 总结调整方案。
- 无第三方 C++ 依赖，命令行版本可直接用 CMake 编译。

## 构建

```bash
cmake -S . -B build -DBDSS_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## 运行

```bash
./build/BDSS \
  --config resources/default_config.json \
  --output simulation_log.csv \
  --people 3500 \
  --report decision_report.md
```

常用参数：

- `--config FILE`：读取 JSON 配置。
- `--output FILE`：输出 CSV。
- `--report FILE`：输出 AI 决策报告。
- `--people N`：预计就餐人数，用于报告中的参数放大建议。
- `--scenario-name NAME`：报告场景名称。
- `--seed N`：覆盖随机种子。
- `--duration SEC`：覆盖仿真时长。
- `--window-count N`：覆盖开放窗口数。
- `--seats N`：覆盖座位数。
- `--arrival-rate X`：覆盖到达率，单位人/分钟。

## CSV 字段

```text
time,total_queue_length,waiting_for_seat_count,occupied_seats,cleaning_seats,total_seats,finished_students,dropped_students,takeaway_students,in_system_students,seat_utilization,window_queue_lengths,window_served_counts
```

其中 `window_queue_lengths` 和 `window_served_counts` 使用分号分隔，便于保存每个窗口的状态。

## 图表生成

```bash
python tools/plot_metrics.py simulation_log.csv --out charts
```

会生成：

- `charts/total_queue_length.png`
- `charts/waiting_for_seat_count.png`
- `charts/seat_utilization.png`

## AI 报告功能对应源码

- `include/utils/DecisionReportGenerator.h`
- `src/utils/DecisionReportGenerator.cpp`
- `main.cpp` 中的 `--people` 和 `--report` 参数
