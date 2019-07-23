cmake_minimum_required(VERSION 3.5)

project(clangtools LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(clangtools
  main.cpp
  clang-analyzer.dividezero.cpp
  clang.unused-parameter.cpp
  clang.unused-parameter.h
  clazy.qgetenv.cpp
  tidy.modernize-use-nullptr.cpp
)

target_link_libraries(clangtools PRIVATE Qt5::Widgets)
