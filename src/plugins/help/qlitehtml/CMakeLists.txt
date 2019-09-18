cmake_minimum_required(VERSION 3.9)

project(QLiteHtml)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 14)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/litehtml/CMakeLists.txt)
    set(ORIG_CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE})
    if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
      set(CMAKE_BUILD_TYPE "RelWithDebInfo")
    else()
      set(CMAKE_BUILD_TYPE "Release")
    endif()
    add_subdirectory(litehtml)
    set(CMAKE_BUILD_TYPE ${ORIG_CMAKE_BUILD_TYPE})
else()
    find_package(litehtml REQUIRED)
endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_library(qlitehtml STATIC
  container_qpainter.cpp container_qpainter.h
  qlitehtmlwidget.cpp qlitehtmlwidget.h
)

target_include_directories(qlitehtml PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qlitehtml PRIVATE Qt5::Widgets litehtml)
