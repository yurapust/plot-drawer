cmake_minimum_required(VERSION 3.1.0)

project(PlotDrawer)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS Concurrent REQUIRED)

add_executable(PlotDrawer
    mainwindow.ui
    mainwindow.cpp
    main.cpp
    plotdrawer.cpp
    dataloader.cpp
    renderthread.cpp
)

target_link_libraries(PlotDrawer Qt5::Widgets)
target_link_libraries(PlotDrawer Qt5::Concurrent)
