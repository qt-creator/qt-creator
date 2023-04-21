cmake_minimum_required(VERSION 3.18)

project(hello-widgets)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(hello-widgets
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

target_link_libraries(hello-widgets PRIVATE Qt6::Widgets)

include(my_add_executable.cmake)

my_add_executable(hello-my-widgets
  main.cpp
  mainwindow.cpp
  mainwindow.h
  mainwindow.ui
)

target_link_libraries(hello-my-widgets PRIVATE Qt6::Widgets)

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS *.cpp *.h *.ui)

add_executable(hello-widgets-glob ${SOURCE_FILES})
target_link_libraries(hello-widgets-glob PRIVATE Qt6::Widgets)
