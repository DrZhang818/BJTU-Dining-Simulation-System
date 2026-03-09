# BDSS: BJTU Dining Simulation System 
# 北京交通大学就餐仿真系统

[![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Framework](https://img.shields.io/badge/Framework-Qt%206-green.svg)](https://www.qt.io/)
[![Build](https://img.shields.io/badge/Build-CMake%20%2F%20Make-brightgreen.svg)](https://cmake.org/)

## 📝 项目简介 (Project Overview)
本项目是北京交通大学《软件综合实训》课程的立项课题。系统通过**离散事件仿真 (Discrete Event Simulation)** 技术，模拟大学食堂在高峰时段的人流、排队及就餐过程。

其核心目标是协助食堂管理部门分析排队瓶颈，通过调整参数（如窗口数、服务效率、座位布局）提供科学的优化建议。

## 🚀 核心功能 (Key Features)
- **动态仿真引擎**: 基于时间步长（Tick）驱动，模拟学生到达、排队、取餐、找座全流程。
- **可视化界面**: 使用 Qt 绘制实时食堂热力图、排队队列及座位占用矩阵。
- **灵活配置**: 支持自定义学生流量分布（如泊松分布）、窗口服务效率及食堂布局。
- **数据分析**: 仿真结束后自动导出 CSV 报表，分析平均等待时间与资源利用率。

## 🛠️ 技术栈 (Tech Stack)
- **语言 (Language)**: C++ 17
- **框架 (Framework)**: Qt 6 (Widgets, Charts)
- **构建工具 (Build System)**: CMake 3.16+, GNU Make
- **开发环境 (IDE)**: VS Code (Linux / SSH Remote)
- **版本控制 (VCS)**: Git / GitHub

## 📂 项目结构 (Project Structure)
```text
.
├── CMakeLists.txt          # 项目构建配置文件
├── main.cpp                # 程序入口
├── src/                    # 源代码 (.cpp)
├── include/                # 头文件 (.h)
├── ui/                     # Qt Designer 界面文件 (.ui)
├── docs/                   # 实训报告、设计文档及原型图
├── resources/              # 图标、配置数据等资源文件
└── build/                  # 编译输出目录 (Git已忽略)
```

## 🔨 快速开始 (Quick Start)

### 1. 环境依赖 (Dependencies)
确保你的 Linux 系统已安装以下基础开发包：
```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-charts-dev qt6-tools-dev-tools
```

### 2. 编译项目 (Build)
在项目根目录下执行以下指令进行编译：
```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### 3. 运行程序 (Run)
编译完成后，直接运行生成的可执行文件：
```bash
./BDSS
```

## 📅 开发计划 (Roadmap)
- [x] **立项阶段**: 环境搭建、技术选型、架构设计。 (Done)
- [ ] **设计阶段**: 完成 UI 原型绘制与核心类 (Student, Window) 接口定义。
- [ ] **开发阶段**: 实现仿真驱动引擎与排队逻辑。
- [ ] **集成阶段**: 加入可视化图表统计与数据导出功能。
- [ ] **总结阶段**: 撰写实训总结报告。

## 📄 许可证 (License)
本项目遵循 [MIT License](LICENSE)。