# 北京交通大学就餐仿真系统（BDSS）

BDSS（Beijing Jiaotong University Dining Simulation System）是面向《软件综合实训》的食堂高峰期离散事件仿真系统。系统模拟学生到达、窗口排队、打饭服务、等待座位、就餐和离场全过程，并输出窗口排队、座位利用率、等待时间等统计指标。

## 特性

- **离散事件仿真引擎** — 学生从到达 → 排队 → 服务 → 等座 → 就餐 → 离开的全生命周期
- **多峰人流模型** — 支持配置多个就餐高峰时段及强度
- **就餐者画像** — 本科生/研究生/教职工三类人群，不同就餐时长
- **窗口偏好** — 学生按窗口类别偏好选择排队队列
- **耐心模型** — 排队过久可离场或切换队列
- **打包外带** — 支持打包比例配置，打包学生不占用座位
- **座位清洁** — 学生离开后座位进入清洁状态
- **结伴用餐** — 支持生成小组，分配邻座
- **座位偏好** — 就近窗口、结伴邻座、陌生人间隔等加权评分
- **Qt6 图形界面** — 实时渲染、配置编辑、统计图表、CSV 导出
- **命令行模式** — 无 GUI 环境亦可运行并输出 CSV
- **严格兼容模式** — 关闭偏好功能后可用旧种子复现结果

## 目录结构

```text
.
├── CMakeLists.txt                 # 构建配置
├── main.cpp                       # 程序入口
├── include/
│   ├── core/                      # 核心仿真：Config, Student, Window, Canteen, SimulationEngine
│   ├── gui/                       # GUI：MainWindow, RenderWidget
│   └── utils/                     # 工具：RandomGenerator, StatisticsLogger
├── src/
│   ├── core/                      # 核心仿真实现
│   ├── gui/                       # GUI 实现
│   └── utils/                     # 工具实现
├── resources/
│   └── default_config.json        # 默认仿真配置
├── tests/
│   └── test_core.cpp              # 核心模块单元测试
└── docs/
    ├── DEVELOPMENT_LOG.md         # 开发日志
    ├── OPTIMIZATION_SUMMARY.md    # 优化总结
    ├── RUN_GUIDE.md               # 快速运行指南
    └── evidence/                  # 验证截图与日志
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

启动图形界面：

```bash
./build/BDSS --gui
```

## 测试

```bash
./build/BDSS_CoreTest
ctest --test-dir build --output-on-failure
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
