cmake_minimum_required(VERSION 3.5)

project(lb LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lb builderutils.h lb.h lb.cpp main.cpp)
target_link_libraries(lb PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

add_executable(tst_lb builderutils.h lb.h lb.cpp tst_lb.cpp)
target_link_libraries(tst_lb PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Test)
