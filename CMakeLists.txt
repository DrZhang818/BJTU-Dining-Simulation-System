cmake_minimum_required(VERSION 3.20)

project(BDSS VERSION 2.0.0 LANGUAGES CXX)

option(BDSS_BUILD_GUI "Build Qt GUI if Qt6 Widgets is available" ON)
option(BDSS_ENABLE_WARNINGS "Enable strict compiler warnings" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt AUTOMOC 在全局级别设置（仅在有 Qt 编译时实际使用）
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(BDSS_CORE_SOURCES
    src/core/Config.cpp
    src/core/Student.cpp
    src/core/Window.cpp
    src/core/Canteen.cpp
    src/core/SimulationEngine.cpp
    src/utils/RandomGenerator.cpp
    src/utils/StatisticsLogger.cpp
)

add_library(BDSSCore STATIC ${BDSS_CORE_SOURCES})
target_include_directories(BDSSCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(BDSSCore PUBLIC cxx_std_23)
target_compile_definitions(BDSSCore PUBLIC BDSS_VERSION="${PROJECT_VERSION}")

if(BDSS_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(BDSSCore PRIVATE /W4 /permissive-)
    else()
        target_compile_options(BDSSCore PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

add_executable(BDSS main.cpp)
target_link_libraries(BDSS PRIVATE BDSSCore)
target_compile_definitions(BDSS PRIVATE BDSS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

set(BDSS_GUI_ENABLED OFF)

# ----- 自动检测 Qt6 -----
# 如果你安装了 Qt6 但 CMake 找不到，可以在这里指定路径：
# list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt@6/lib/cmake")

if(BDSS_BUILD_GUI)
    find_package(Qt6 QUIET COMPONENTS Widgets Charts)
    if(Qt6_FOUND AND TARGET Qt6::Widgets)
        set(BDSS_GUI_ENABLED ON)
        target_sources(BDSS PRIVATE
            include/gui/MainWindow.h
            include/gui/RenderWidget.h
            src/gui/MainWindow.cpp
            src/gui/RenderWidget.cpp
        )
        target_link_libraries(BDSS PRIVATE Qt6::Widgets)
        if(TARGET Qt6::Charts)
            target_link_libraries(BDSS PRIVATE Qt6::Charts)
            target_compile_definitions(BDSS PRIVATE BDSS_HAS_CHARTS)
            message(STATUS "Qt6 Charts found: charts tab enabled.")
        else()
            message(STATUS "Qt6 Charts not found: charts tab will show placeholder.")
        endif()
        target_compile_definitions(BDSS PRIVATE BDSS_HAS_GUI)
        message(STATUS "Qt6 Widgets found: GUI build enabled.")
    else()
        message(STATUS "Qt6 Widgets not found: BDSS will be built in headless mode.")
    endif()
endif()

add_executable(BDSS_CoreTest tests/test_core.cpp)
target_link_libraries(BDSS_CoreTest PRIVATE BDSSCore)
target_compile_definitions(BDSS_CoreTest PRIVATE BDSS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

if(BDSS_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(BDSS PRIVATE /W4 /permissive-)
        target_compile_options(BDSS_CoreTest PRIVATE /W4 /permissive-)
    else()
        target_compile_options(BDSS PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(BDSS_CoreTest PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

enable_testing()
add_test(NAME BDSS_CoreTest COMMAND BDSS_CoreTest)
