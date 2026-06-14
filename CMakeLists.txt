cmake_minimum_required(VERSION 3.20)

project(BDSS VERSION 3.1.0 LANGUAGES CXX)

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
    src/core/SimulationEngine.cpp
    src/utils/StatisticsLogger.cpp
    src/utils/DecisionReportGenerator.cpp
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
