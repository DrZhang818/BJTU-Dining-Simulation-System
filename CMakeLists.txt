# 1. 建议将最低版本提升到 3.20 以支持 C++23
cmake_minimum_required(VERSION 3.20)

# 2. 项目名称
project(CanteenSim VERSION 1.0 LANGUAGES CXX)

# 3. 指定 C++ 标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 针对 Windows 的特别优化：处理 DLL 文件 ---
# 在 Windows 上运行 Qt 程序需要 .dll 文件，这段代码能自动把 dll 复制到 exe 所在目录
if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

# 4. 自动处理 Qt 核心机制
set(CMAKE_AUTOMOC ON) 
set(CMAKE_AUTOUIC ON) 
set(CMAKE_AUTORCC ON) 

# 5. 寻找 Qt 库
find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)

# 6. 收集源文件 (必须先定义 SOURCES 变量)
set(SOURCES
    main.cpp
    src/core/Config.cpp
    src/core/Student.cpp
)

# 7. 生成可执行文件 (这步是“诞生”，必须放在前面！)
add_executable(CanteenSim ${SOURCES})

# 8. 包含头文件目录 (现在 CanteenSim 已经存在了，可以给它配置路径了)
target_include_directories(CanteenSim PRIVATE include)

# 9. 链接 Qt 库
target_link_libraries(CanteenSim PRIVATE Qt6::Widgets Qt6::Charts)