# 1. 指定 CMake 最低版本要求
cmake_minimum_required(VERSION 3.16)

# 2. 项目名称和版本
project(CanteenSim VERSION 1.0 LANGUAGES CXX)

# 3. 指定 C++ 标准 (推荐 C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 4. 自动处理 Qt 的核心机制 (关键！)
set(CMAKE_AUTOMOC ON) # 处理 信号与槽 (Signals & Slots)
set(CMAKE_AUTOUIC ON) # 处理 .ui 界面文件
set(CMAKE_AUTORCC ON) # 处理 资源文件 (.qrc)

# 5. 寻找 Qt 库 (由于你用 Linux 系统安装，通常能自动找到)
find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)

# 6. 包含头文件目录
include_directories(include)

# 7. 收集源文件 (把你的源文件都列在这)
set(SOURCES
    main.cpp
    src/StudentManager.cpp
    src/CanteenMap.cpp
)

# 8. 生成可执行文件
add_executable(CanteenSim ${SOURCES})

# 9. 链接 Qt 库
target_link_libraries(CanteenSim PRIVATE Qt6::Widgets Qt6::Charts)