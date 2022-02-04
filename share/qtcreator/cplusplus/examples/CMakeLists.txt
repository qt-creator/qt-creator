cmake_minimum_required(VERSION 3.5)

project(examples LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets REQUIRED)

set_property(SOURCE clazy_example.cpp icontest.cpp PROPERTY SKIP_AUTOMOC ON)

add_executable(examples
  clazy_example.cpp
  icontest.cpp
  tidy_example.cpp
  tidy_example.h
)

target_link_libraries(examples PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
