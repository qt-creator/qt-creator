cmake_minimum_required(VERSION 3.10)

project(QLiteHtml)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/litehtml/CMakeLists.txt)
    set(ORIG_FPIC ${CMAKE_POSITION_INDEPENDENT_CODE})
    if (WIN32)
      set(LITEHTML_UTF8 ON CACHE BOOL "")
    endif()
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_subdirectory(litehtml EXCLUDE_FROM_ALL)
    set(CMAKE_POSITION_INDEPENDENT_CODE "${ORIG_FPIC}")
    # force optimized litehtml even in debug
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      # except for windows
      if (NOT WIN32)
        target_compile_options(gumbo PRIVATE -O2)
        target_compile_options(litehtml PRIVATE -O2)
      endif()
    endif()
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
