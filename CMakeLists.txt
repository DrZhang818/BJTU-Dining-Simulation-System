cmake_minimum_required(VERSION 3.20)

project(BDSS VERSION 3.0.0 LANGUAGES CXX)

option(BDSS_BUILD_GUI "Build Qt GUI if Qt6 Widgets is available" ON)
option(BDSS_BUILD_TESTS "Build core regression tests" ON)
option(BDSS_ENABLE_WARNINGS "Enable compiler warnings" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(BDSS_CORE_SOURCES
    src/core/Config.cpp
    src/core/Student.cpp
    src/core/Window.cpp
    src/core/Canteen.cpp
    src/core/SimulationEngine.cpp
    src/utils/SimpleJson.cpp
    src/utils/StatisticsLogger.cpp
)

add_library(BDSSCore STATIC ${BDSS_CORE_SOURCES})
target_include_directories(BDSSCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(BDSSCore PUBLIC BDSS_VERSION="${PROJECT_VERSION}")

add_executable(BDSS main.cpp)
target_link_libraries(BDSS PRIVATE BDSSCore)
target_compile_definitions(BDSS PRIVATE BDSS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

if(BDSS_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(BDSSCore PRIVATE /W4 /permissive-)
        target_compile_options(BDSS PRIVATE /W4 /permissive-)
    else()
        target_compile_options(BDSSCore PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(BDSS PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if(BDSS_BUILD_GUI)
    find_package(Qt6 QUIET COMPONENTS Widgets)
    if(Qt6_FOUND AND TARGET Qt6::Widgets)
        set_target_properties(BDSS PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
        target_sources(BDSS PRIVATE
            include/gui/MainWindow.h
            include/gui/RenderWidget.h
            include/gui/ChartWidget.h
            src/gui/MainWindow.cpp
            src/gui/RenderWidget.cpp
            src/gui/ChartWidget.cpp
        )
        target_link_libraries(BDSS PRIVATE Qt6::Widgets)
        target_compile_definitions(BDSS PRIVATE BDSS_HAS_GUI)
        message(STATUS "Qt6 Widgets found: GUI build enabled.")
    else()
        message(STATUS "Qt6 Widgets not found: BDSS will be built in headless mode.")
    endif()
endif()

if(BDSS_BUILD_TESTS)
    add_executable(BDSS_CoreTest tests/test_core.cpp)
    target_link_libraries(BDSS_CoreTest PRIVATE BDSSCore)
    target_compile_definitions(BDSS_CoreTest PRIVATE BDSS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    if(BDSS_ENABLE_WARNINGS)
        if(MSVC)
            target_compile_options(BDSS_CoreTest PRIVATE /W4 /permissive-)
        else()
            target_compile_options(BDSS_CoreTest PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endif()
    enable_testing()
    add_test(NAME BDSS_CoreTest COMMAND BDSS_CoreTest)
endif()
