cmake_minimum_required(VERSION 3.20)

project(BDSS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BDSS_BUILD_GUI "Build Qt GUI if Qt6 Widgets is available" ON)

set(BDSS_CORE_SOURCES
    src/core/Config.cpp
    src/core/Student.cpp
    src/core/Window.cpp
    src/core/Canteen.cpp
    src/core/SimulationEngine.cpp
    src/utils/RandomGenerator.cpp
    src/utils/StatisticsLogger.cpp
)

set(BDSS_GUI_SOURCES
    include/gui/MainWindow.h
    include/gui/RenderWidget.h
    src/gui/MainWindow.cpp
    src/gui/RenderWidget.cpp
)

set(BDSS_GUI_ENABLED OFF)

if(BDSS_BUILD_GUI)
    find_package(Qt6 QUIET COMPONENTS Widgets)

    if(Qt6_FOUND AND TARGET Qt6::Widgets)
        set(BDSS_GUI_ENABLED ON)
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTOUIC ON)
        set(CMAKE_AUTORCC ON)
        message(STATUS "Qt6 Widgets found: GUI build enabled.")
    else()
        message(WARNING "Qt6 Widgets not found: BDSS will be built in headless mode.")
    endif()
endif()

add_executable(BDSS
    main.cpp
    ${BDSS_CORE_SOURCES}
)

target_include_directories(BDSS PRIVATE include)
target_compile_features(BDSS PRIVATE cxx_std_23)

if(BDSS_GUI_ENABLED)
    target_sources(BDSS PRIVATE ${BDSS_GUI_SOURCES})
    target_link_libraries(BDSS PRIVATE Qt6::Widgets)
    target_compile_definitions(BDSS PRIVATE BDSS_HAS_GUI)
endif()

add_executable(BDSS_CoreTest
    tests/test_core.cpp
    ${BDSS_CORE_SOURCES}
)

target_include_directories(BDSS_CoreTest PRIVATE include)
target_compile_features(BDSS_CoreTest PRIVATE cxx_std_23)