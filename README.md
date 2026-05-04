# 北京交通大学就餐仿真系统  
# BJTU Dining Simulation System, BDSS

[![Language](https://img.shields.io/badge/Language-C%2B%2B23-red.svg)](https://en.cppreference.com/w/cpp/23)
[![Build](https://img.shields.io/badge/Build-CMake%203.20%2B-brightgreen.svg)](https://cmake.org/)

---

## 一、项目简介

北京交通大学就餐仿真系统是《软件综合实训》课程的小组开发项目。本系统基于离散事件仿真思想，模拟学生在食堂高峰时段的完整就餐流程，包括学生到达食堂、窗口排队、打饭、寻找座位、就餐和离开食堂等环节。

本项目的目标是通过仿真方式观察食堂在不同参数配置下的运行状态，分析窗口排队压力、座位资源利用率以及学生平均等待时间等指标，为食堂管理和资源优化提供数据参考。

当前版本已完成核心仿真逻辑闭环，并新增 Qt GUI 初步界面，包括参数配置页、实时监控页和统计图表占位页。实时监控页能够展示窗口排队情况和餐桌矩阵占用状态。统计图表页目前仍为占位页面，后续将基于 CSV 或 StatisticsLogger 数据绘制趋势图。

---

## 二、核心功能

当前开发阶段已实现以下功能：

1. 学生随机到达模拟  
2. 学生窗口排队逻辑  
3. 窗口打饭服务逻辑  
4. 学生等待空座逻辑  
5. 餐桌矩阵座位管理  
6. 学生就餐和离场逻辑  
7. 仿真时间步长驱动  
8. 统计数据记录  
9. CSV 文件导出  
10. 核心模块单元测试  

---

## 三、技术栈

| 类别 | 技术 |
|---|---|
| 开发语言 | C++23 |
| 构建工具 | CMake 3.20+ |
| 开发环境 | VS Code / Remote SSH |
| 数据结构 | STL Queue、Vector、Shared Pointer |
| 测试方式 | assert 断言测试 |
| 数据输出 | CSV 文件 |
| 后续 GUI | Qt 6 |

---

## 四、项目目录结构

```text
BJTU-Dining-Simulation-System/
├── CMakeLists.txt
├── main.cpp
├── README.md
├── resources/
│   └── default_config.json
├── include/
│   ├── core/
│   │   ├── Config.h
│   │   ├── Student.h
│   │   ├── Window.h
│   │   ├── Canteen.h
│   │   └── SimulationEngine.h
│   └── utils/
│       ├── RandomGenerator.h
│       └── StatisticsLogger.h
├── src/
│   ├── core/
│   │   ├── Config.cpp
│   │   ├── Student.cpp
│   │   ├── Window.cpp
│   │   ├── Canteen.cpp
│   │   └── SimulationEngine.cpp
│   └── utils/
│       ├── RandomGenerator.cpp
│       └── StatisticsLogger.cpp
└── tests/
    └── test_core.cpp
```

---

## 五、核心模块说明

### 1. Config 配置模块

对应文件：

```text
include/core/Config.h
src/core/Config.cpp
resources/default_config.json
```

该模块负责保存和读取系统仿真参数，包括窗口数量、餐桌行列数、仿真总时长、学生到达率、平均打饭时间、平均就餐时间、高峰时段参数等。

---

### 2. Student 学生实体模块

对应文件：

```text
include/core/Student.h
src/core/Student.cpp
```

该模块用于描述学生在仿真系统中的生命周期。学生状态包括：

```text
Arrived → Queuing → Serving → WaitingForSeat → Dining → Left
```

系统会记录学生的到达时间、开始排队时间、开始服务时间、打饭结束时间、开始就餐时间和离开时间，用于计算排队等待时间、座位等待时间和总停留时间。

---

### 3. Window 窗口排队模块

对应文件：

```text
include/core/Window.h
src/core/Window.cpp
```

该模块负责模拟食堂窗口的排队和服务过程。每个窗口维护一个学生队列，每次仿真时间推进时，窗口会处理队首学生的打饭剩余时间。学生打饭完成后，会进入等待座位队列。

窗口支持效率系数设置，可模拟不同窗口服务速度的差异。

---

### 4. Canteen 餐桌管理模块

对应文件：

```text
include/core/Canteen.h
src/core/Canteen.cpp
```

该模块使用二维矩阵表示食堂座位状态。系统会为完成打饭的学生寻找空座，如果存在空座，学生进入就餐状态；如果没有空座，则进入等待座位队列。

学生就餐时间归零后，系统会释放对应座位，并将学生状态设置为离开。

---

### 5. SimulationEngine 仿真驱动模块

对应文件：

```text
include/core/SimulationEngine.h
src/core/SimulationEngine.cpp
```

该模块是系统核心控制器。每次调用 `tick()` 表示仿真时间推进 1 秒。执行流程如下：

```text
1. 根据到达率生成新学生
2. 为学生选择排队人数最少的窗口
3. 推进窗口打饭服务
4. 将完成打饭的学生加入等待座位队列
5. 推进餐桌上学生的就餐过程
6. 释放完成就餐的座位
7. 为等待座位的学生分配空座
8. 记录当前时刻统计数据
```

当学生不再生成，窗口队列为空，等待座位队列为空，并且餐桌没有学生时，仿真结束。

---

### 6. RandomGenerator 随机数模块

对应文件：

```text
include/utils/RandomGenerator.h
src/utils/RandomGenerator.cpp
```

该模块封装随机数生成逻辑，主要用于：

```text
泊松分布：模拟每秒到达学生数量
正态分布：模拟打饭时间和就餐时间
均匀分布：生成指定范围内随机值
```

系统支持设置固定随机种子，便于调试和测试结果复现。

---

### 7. StatisticsLogger 统计日志模块

对应文件：

```text
include/utils/StatisticsLogger.h
src/utils/StatisticsLogger.cpp
```

该模块负责记录仿真过程中的统计数据，并在仿真结束后导出 `simulation_log.csv`。

统计指标包括：

```text
完成就餐学生数
最大窗口排队长度
最大等待座位人数
平均窗口排队等待时间
平均等待座位时间
平均打饭服务时间
平均总停留时间
平均座位利用率
```

---

## 六、配置文件说明

默认配置文件位于：

```text
resources/default_config.json
```

示例配置如下：

```json
{
  "windowCount": 5,
  "tableRows": 10,
  "tableCols": 10,
  "totalSimulationTime": 3600,

  "arrivalRate": 5.0,
  "avgServiceTime": 20,
  "avgDiningTime": 900,

  "serviceStddev": 5.0,
  "diningStddev": 180.0,

  "arrivalPattern": "RushHour",
  "windowEfficiency": "Variable",

  "rushHourStart": 900,
  "rushHourEnd": 1800,
  "rushHourMultiplier": 2.0,

  "randomSeed": 42
}
```

---

## 七、编译与运行

### 1. 编译项目

在项目根目录执行：

```bash
rm -rf build
cmake -S . -B build
cmake --build build
```

---

### 2. 运行主程序

```bash
./build/BDSS
```

程序运行后会输出仿真统计结果，并生成：

```text
simulation_log.csv
```

示例输出：

```text
========== BDSS Simulation Summary ==========
Finished students: 373
Max queue length: 11
Max waiting-for-seat count: 50
Average queue wait time: 2.08847 seconds
Average seat wait time: 145.52 seconds
Average service time: 22.2064 seconds
Average total time in canteen: 1061.4 seconds
Average seat utilization: 68.7394%
============================================
CSV exported to simulation_log.csv
```

---

### 3. 运行单元测试

```bash
./build/BDSS_CoreTest
```

测试通过时会显示：

```text
[PASS] Student state and time calculation
[PASS] Window queue and service
[PASS] Canteen seat allocation and release
[PASS] RandomGenerator range test
[PASS] SimulationEngine full-process test
[INFO] CSV exported to simulation_log.csv

All core tests passed.
```

---

## 八、CSV 输出说明

程序运行完成后会生成：

```text
simulation_log.csv
```

CSV 文件字段如下：

```text
time,total_queue_length,waiting_for_seat_count,occupied_seats,finished_students,seat_utilization
```

字段含义：

| 字段 | 含义 |
|---|---|
| time | 当前仿真时间 |
| total_queue_length | 所有窗口总排队人数 |
| waiting_for_seat_count | 等待座位人数 |
| occupied_seats | 当前占用座位数 |
| finished_students | 已完成就餐并离开的学生数量 |
| seat_utilization | 当前座位利用率 |

该 CSV 文件可使用 Excel、WPS 或其他数据分析工具打开，用于绘制排队人数变化曲线、座位利用率曲线等统计图表。

---

## 九、单元测试说明

核心测试代码位于：

```text
tests/test_core.cpp
```

测试覆盖内容包括：

| 测试模块 | 测试内容 |
|---|---|
| Student | 学生状态切换和时间计算 |
| Window | 学生入队、窗口服务和出队 |
| Canteen | 座位分配、就餐推进和座位释放 |
| RandomGenerator | 随机数范围和泊松分布生成 |
| SimulationEngine | 完整仿真流程 |
| StatisticsLogger | 统计结果输出和 CSV 导出 |

---

## 十、小组开发分工

| 成员 | 主要任务 |
|---|---|
| 成员A：张恩崎 | 核心仿真逻辑、SimulationEngine、CMake 构建 |
| 成员B：王政 | Qt GUI 可视化、主界面、交互控制 |
| 成员C：孜克茹拉·阿不都克依木 | 单元测试、统计日志、CSV 导出、文档整理 |

---

## 十一、当前开发阶段成果

当前开发阶段已经完成：

```text
1. 核心仿真逻辑闭环
2. 学生状态流转
3. 窗口排队和打饭服务
4. 餐桌矩阵座位管理
5. 等待座位队列
6. 仿真时间步长驱动
7. 随机学生生成
8. 统计数据记录
9. simulation_log.csv 导出
10. 核心模块单元测试
```

---

## 十二、当前不足与后续计划

当前版本仍有以下不足：

```text
1. 尚未实现 Qt 图形界面。
2. 尚未实现实时动画展示。
3. 尚未实现统计图表页面。
4. 配置文件解析方式较简单。
5. 学生寻座策略目前采用顺序查找空座。
6. 窗口选择策略目前主要依据队列长度。
```

后续计划：

```text
1. 使用 Qt Widgets 实现主窗口界面。
2. 使用 QTimer 驱动仿真刷新。
3. 使用 QPainter 绘制窗口队列和餐桌矩阵。
4. 增加参数配置页面。
5. 增加统计图表展示页面。
6. 完成系统集成测试和最终报告。
```

---

## 十三、提交材料说明

开发阶段建议提交以下内容：

```text
1. 项目源代码压缩包
2. README.md
3. simulation_log.csv
4. 核心模块单元测试报告
5. 开发阶段个人实训报告
6. 小组沟通交流记录
7. 小组开发任务划分说明
```

---

## 十四、许可证

本项目仅用于《软件综合实训》课程学习与教学交流。