# BJTU Dining Simulation System

北京交通大学就餐仿真系统。项目使用 C++17 实现离散时间/离散事件风格的食堂排队、服务、找座、就餐、清洁、外带、群体就餐和窗口偏好模型，并提供命令行批量仿真与 Qt6 图形界面。

## 这版主要改进

- 修复“前端参数显示已变，但内部逻辑没有跟上”的问题：`SimulationEngine::updateConfig()` 会在应用参数时同步内核配置，并重新构建餐口数组；新增窗口会立即参与 `chooseWindowIndex()` 排队路由。
- 新增回归测试：覆盖初始增加窗口、运行中增加窗口、配置归一化、CSV 导出、外带不占座等核心场景。
- 优化配置系统：支持 `windowProfiles`、`studentTypes`、多段高峰、窗口效率、耐心换队、外带打包、清洁、群体就餐和座位偏好；同时兼容部分旧字段名。
- 优化 UI：提供参数同步状态、应用参数按钮、实时餐口排队卡片、座位矩阵、统计卡片、趋势曲线和 CSV 导出。
- 优化工程结构：CMake 选项更清晰，Qt 不存在时自动构建命令行版本，CI 会编译、测试并做 smoke run。

## 构建

命令行版本不依赖 Qt：

```bash
cmake -S . -B build -DBDSS_BUILD_GUI=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

图形界面版本需要 Qt6 Widgets：

```bash
cmake -S . -B build -DBDSS_BUILD_GUI=ON
cmake --build build --parallel
./build/BDSS --gui --config resources/default_config.json
```

如果当前机器没有 Qt6 Widgets，CMake 会自动退回命令行版本。

## 命令行运行

```bash
./build/BDSS --headless \
  --config resources/default_config.json \
  --output simulation_log.csv \
  --seed 42 \
  --duration 3600 \
  --window-count 10
```

常用参数：

- `--config FILE`：读取 JSON 配置。
- `--output FILE`：输出 CSV。
- `--seed N`：覆盖随机种子。
- `--duration SEC`：覆盖仿真时长，单位秒。
- `--window-count N`：覆盖餐口数量。
- `--arrival-rate X`：覆盖到达率，单位人/分钟。

## 配置文件

默认配置在 `resources/default_config.json`。高峰时间使用“仿真开始后的秒数”，例如 `900` 表示仿真开始后第 15 分钟。

示例配置：

```json
{
  "windowCount": 10,
  "arrivalRate": 7.5,
  "arrivalPattern": "RushPeaks",
  "rushPeaks": [
    { "start": 900, "end": 2400, "multiplier": 2.4 }
  ],
  "enableWindowPreferences": true,
  "windowCategories": ["米饭套餐", "面食", "麻辣烫", "清真窗口"],
  "enableTakeaway": true,
  "takeawayRate": 0.18
}
```

更多说明见：

- `docs/OPTIMIZATION_REPORT.md`
- `docs/MODEL_ASSUMPTIONS.md`
- `docs/RUN_GUIDE.md`

## CSV 输出

CSV 包含整体指标和每个窗口指标：

```text
time,total_queue_length,waiting_for_seat_count,occupied_seats,cleaning_seats,total_seats,finished_students,dropped_students,takeaway_students,in_system_students,seat_utilization,window_queue_lengths,window_served_counts
```

`window_queue_lengths` 和 `window_served_counts` 使用分号分隔，例如 `"0;2;1;0;5"`。
