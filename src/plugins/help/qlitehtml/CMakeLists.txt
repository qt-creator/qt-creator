cmake_minimum_required(VERSION 3.9)

project(QLiteHtml)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 14)

find_package(litehtml REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_library(qlitehtml STATIC
  container_qpainter.cpp container_qpainter.h
  qlitehtmlwidget.cpp qlitehtmlwidget.h
)

target_include_directories(qlitehtml PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qlitehtml PRIVATE Qt5::Widgets litehtml)
