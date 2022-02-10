cmake_minimum_required(VERSION 3.16)

project(manual_test_debugger_gui LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(manual_test_debugger_gui
  mainwindow.cpp mainwindow.h mainwindow.ui
  tst_gui.cpp
)
target_link_libraries(manual_test_debugger_gui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
