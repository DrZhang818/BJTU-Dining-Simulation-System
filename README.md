# BDSS: BJTU Dining Simulation System 
# 北京交通大学就餐仿真系统

[![Language](https://img.shields.io/badge/Language-C%2B%2B23-red.svg)](https://en.cppreference.com/w/cpp/23)
[![Framework](https://img.shields.io/badge/Framework-Qt%206-green.svg)](https://www.qt.io/)
[![Build](https://img.shields.io/badge/Build-CMake%203.20%2B-brightgreen.svg)](https://cmake.org/)

## 📝 项目简介 (Project Overview)
本项目是北京交通大学《软件综合实训》课程的立项课题。系统通过**离散事件仿真 (Discrete Event Simulation)** 技术，模拟大学食堂在高峰时段的人流、排队及就餐过程。

其核心目标是协助食堂管理部门分析排队瓶颈，通过调整参数（如窗口数、服务效率、座位布局）提供科学的优化建议。

## 🚀 核心功能 (Key Features)
- **动态仿真引擎**: 基于时间步长（Tick）驱动，模拟学生到达、排队、取餐、找座全流程。
- **可视化界面**: 使用 Qt 绘制实时食堂热力图、排队队列及座位占用矩阵。
- **灵活配置**: 支持自定义学生流量分布（如泊松分布）、窗口服务效率及食堂布局。
- **数据分析**: 仿真结束后自动导出 CSV 报表，分析平均等待时间与资源利用率。

## 🛠️ 技术栈 (Tech Stack)
- **语言 (Language)**: C++ 23
- **框架 (Framework)**: Qt 6 (Widgets, Charts)
- **构建工具 (Build System)**: CMake 3.20+, GNU Make
- **开发环境 (IDE)**: VS Code (Linux / SSH Remote)
- **版本控制 (VCS)**: Git / GitHub

## 📂 项目结构 (Project Structure)
```text
BDSS/
├── CMakeLists.txt
├── main.cpp                        # 程序的绝对入口
├── resources/
│   └── default_config.json         # [模块1] 默认配置文件
├── ui/
│   └── MainWindow.ui               # [模块5] Qt Designer 界面文件
├── include/
│   ├── core/                       # 核心业务逻辑层 (纯C++)
│   │   ├── Config.h                # [模块1] 配置数据结构
│   │   ├── Student.h               #[模块3.1] 学生实体类
│   │   ├── Window.h                # [模块3.2] 窗口队列实体类
│   │   ├── Canteen.h               # [模块3.3/4] 餐桌矩阵与空间管理类
│   │   └── SimulationEngine.h      # [模块2] 核心驱动引擎
│   ├── utils/                      # 工具层
│   │   ├── RandomGenerator.h       #[模块3.1] 随机数生成工具 (封装正态/泊松分布)
│   │   └── StatisticsLogger.h      # [模块6] 数据记录与 CSV 导出
│   └── gui/                        # 可视化与交互层 (依赖 Qt)
│       ├── MainWindow.h            # [模块5] 主窗口控制器
│       └── RenderWidget.h          # [模块5] 负责绘制动画的自定义画布组件
└── src/
    ├── core/
    │   ├── Config.cpp
    │   ├── Student.cpp
    │   ├── Window.cpp
    │   ├── Canteen.cpp
    │   └── SimulationEngine.cpp
    ├── utils/
    │   ├── RandomGenerator.cpp
    │   └── StatisticsLogger.cpp
    └── gui/
        ├── MainWindow.cpp
        └── RenderWidget.cpp
```

## 🔨 快速开始 (Quick Start)

好的，为了让 Windows 端的组员能够顺利上手，我们需要在 README 中提供清晰、分步骤的安装说明。Windows 的配置不像 Linux 一行命令就能解决，通常涉及安装包的下载和环境变量的配置。

以下是建议补充到 README 中的 Windows 环境依赖部分，采用了更符合 Windows 用户习惯的描述方式：

---

### 1. 环境依赖 (Dependencies)

#### 🐧 Linux (Ubuntu/Debian)
执行以下指令安装基础开发包：
```bash
sudo add-apt-repository ppa:ubuntu-toolchain-r/test # 获取 GCC 13
sudo apt update
sudo apt install g++-13 cmake qt6-base-dev qt6-charts-dev qt6-tools-dev-tools
```

#### 🪟 Windows
Windows 建议使用 **Visual Studio 2022** 或 **VS Code + MinGW**。

1.  **编译器 & IDE**:
    *   下载并安装 [Visual Studio 2022 Community](https://visualstudio.microsoft.com/zh-hans/vs/)。
    *   安装时务必勾选 **“使用 C++ 的桌面开发”**（确保包含最新的 MSVC 编译器以支持 C++23）。
2.  **Qt 6 框架**:
    *   通过 [Qt Online Installer](https://www.qt.io/download-open-source) 安装。
    *   选择组件时，勾选 `Qt 6.x` 下的 `MSVC 2022 64-bit` 以及 `Additional Libraries` 中的 **`Qt Charts`**。
3.  **CMake**:
    *   下载并安装 [CMake](https://cmake.org/download/) (3.20 或更高版本)，安装时选择 "Add CMake to the system PATH"。
4.  **VS Code 插件** (推荐):
    *   `C/C++ Extension Pack`
    *   `CMake Tools`
    *   `Qt Allman Style` (可选，用于格式化)

> **Windows 编译注意事项**:
> 如果 CMake 提示找不到 Qt6，请在 VS Code 的 `settings.json` 中配置 `cmake.configureSettings`，手动指定 Qt 的 CMake 路径：
> ```json
> "cmake.configureSettings": {
>     "CMAKE_PREFIX_PATH": "C:/Qt/6.x.x/msvc2022_64/lib/cmake"
> }
> ```

### 2. 编译项目 (Build)
在项目根目录下执行以下指令进行编译：
```bash
mkdir -p build && cd build
cmake .. -DCMAKE_CXX_COMPILER=g++-13
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