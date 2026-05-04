cmake_minimum_required(VERSION 3.20)

project(BDSS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BDSS_CORE_SOURCES
    src/core/Config.cpp
    src/core/Student.cpp
    src/core/Window.cpp
    src/core/Canteen.cpp
    src/core/SimulationEngine.cpp
    src/utils/RandomGenerator.cpp
    src/utils/StatisticsLogger.cpp
)

add_executable(BDSS
    main.cpp
    ${BDSS_CORE_SOURCES}
)

target_include_directories(BDSS PRIVATE include)

target_compile_features(BDSS PRIVATE cxx_std_23)

add_executable(BDSS_CoreTest
    tests/test_core.cpp
    ${BDSS_CORE_SOURCES}
)

target_include_directories(BDSS_CoreTest PRIVATE include)

target_compile_features(BDSS_CoreTest PRIVATE cxx_std_23)