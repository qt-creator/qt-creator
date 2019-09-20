cmake_minimum_required(VERSION 3.9)

project(QLiteHtml)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/litehtml/CMakeLists.txt)
    set(ORIG_FPIC ${CMAKE_POSITION_INDEPENDENT_CODE})
    set(ORIG_CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE})
    if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
      set(CMAKE_BUILD_TYPE "RelWithDebInfo")
    else()
      set(CMAKE_BUILD_TYPE "Release")
    endif()
    if (WIN32)
      set(LITEHTML_UTF8 ON)
    endif()
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_subdirectory(litehtml)
    set(CMAKE_BUILD_TYPE ${ORIG_CMAKE_BUILD_TYPE})
    set(CMAKE_POSITION_INDEPENDENT_CODE "${ORIG_FPIC}")
else()
    find_package(litehtml REQUIRED)
endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 14)

add_library(qlitehtml STATIC
  container_qpainter.cpp container_qpainter.h
  qlitehtmlwidget.cpp qlitehtmlwidget.h
)

target_include_directories(qlitehtml PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qlitehtml PRIVATE Qt5::Widgets litehtml)
set_target_properties(qlitehtml PROPERTIES POSITION_INDEPENDENT_CODE ON)
