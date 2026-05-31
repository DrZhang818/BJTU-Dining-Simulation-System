# 北京交通大学就餐仿真系统（BDSS）

BDSS 是面向《软件综合实训》的食堂高峰期离散事件仿真系统。系统模拟学生到达、窗口排队、打饭服务、等待座位、就餐和离场全过程，并输出窗口排队、座位利用率、等待时间等统计指标。

## 本次优化重点

本版本在原有核心闭环基础上做了工程化整理和可靠性增强：

1. 将核心仿真逻辑拆分为 `BDSSCore` 静态库，主程序和测试程序复用同一套核心代码。
2. 增加配置参数校验，避免窗口数、座位数、时长、到达率等非法输入导致异常结果。
3. 明确 `arrivalRate` 为“每分钟平均到达人数”，仿真时自动换算为每秒泊松到达强度，更符合 1 小时高峰场景。
4. 增强命令行入口，支持 `--config`、`--output`、`--seed`、`--duration`、`--headless` 等参数。
5. 保留 Qt GUI 可选构建能力；无 Qt 环境时自动构建 headless 版本，降低提交和验收风险。
6. 增加 `ctest` 集成和更完整的断言测试，覆盖配置、学生生命周期、窗口、座位、随机数、统计日志和完整仿真流程。
7. CSV 输出保留课程报告需要的核心字段：`time,total_queue_length,waiting_for_seat_count,occupied_seats,finished_students,seat_utilization`。
8. 使用固定随机种子保证测试和报告数据可复现。

## 目录结构

```text
BDSS-Optimized/
├── CMakeLists.txt
├── main.cpp
├── include/
│   ├── core/
│   ├── gui/
│   └── utils/
├── src/
│   ├── core/
│   ├── gui/
│   └── utils/
├── resources/default_config.json
├── tests/test_core.cpp
├── docs/OPTIMIZATION_SUMMARY.md
└── evidence/
```

## 编译

```bash
rm -rf build
cmake -S . -B build -DBDSS_BUILD_GUI=OFF
cmake --build build
```

如本机安装了 Qt 6 Widgets，也可以不关闭 GUI：

```bash
cmake -S . -B build -DBDSS_BUILD_GUI=ON
cmake --build build
```

## 运行

```bash
./build/BDSS --headless --config resources/default_config.json --output simulation_log.csv
```

可选参数：

```bash
./build/BDSS --headless --seed 2026 --duration 1800 --output result.csv
```

## 测试

```bash
./build/BDSS_CoreTest
ctest --test-dir build --output-on-failure
```

测试通过时会看到类似输出：

```text
[PASS] Student state and time calculation
[PASS] Window queue and service
[PASS] Canteen seat allocation and release
[PASS] Config parse and validation
[PASS] RandomGenerator range test
[PASS] SimulationEngine full-process test
[PASS] StatisticsLogger CSV export

All core tests passed.
```

## CSV 字段

| 字段 | 含义 |
|---|---|
| `time` | 当前仿真时间，单位秒 |
| `total_queue_length` | 所有窗口可见排队人数总和 |
| `waiting_for_seat_count` | 打饭完成后正在等待座位的人数 |
| `occupied_seats` | 当前已占用座位数 |
| `finished_students` | 已完成就餐并离开的人数 |
| `seat_utilization` | 当前座位利用率百分比 |

## 运行环境建议

- CMake 3.20+
- 支持 C++23 的编译器，例如 GCC 13+、Clang 16+、MSVC 2022
- Qt 6 Widgets 可选，仅用于 GUI

