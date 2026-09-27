cmake_minimum_required(VERSION 3.20)

project(DataCollect2D
    LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


# Find Qt5 components
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

add_executable(${PROJECT_NAME}
    src/source/main.cpp
    src/source/Process.cpp
    src/source/MainWindow.cpp
    src/header/MainWindow.h
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/header
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    m
    Qt5::Core
    Qt5::Widgets
)
