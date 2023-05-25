cmake_minimum_required(VERSION 3.16)

set(GOOGLETEST_DIR ${CMAKE_CURRENT_LIST_DIR}/3rdparty/googletest)

find_package(Googletest MODULE)
find_package(GoogleBenchmark MODULE)

if (NOT Googletest_FOUND)
  message(STATUS "Googletest was not found. Please update the submodules with `git submodule update --init --recursive`.")
  message(STATUS "Have a look at cmake/FindGoogletest.cmake file for more details.")
  message(STATUS "unit module will be skipped.")
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
enable_testing()

# Needed for pch
set(QtCreator_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../")
option(BUILD_WITH_PCH "Build with precompiled headers" ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(IMPLICIT_DEPENDS Qt::Test)

include(QtCreatorIDEBranding)
include(QtCreatorAPI)

set(WITH_TESTS ON)

find_package(Clang MODULE)
find_package(Qt5
  COMPONENTS
    Gui Core Core5Compat Widgets Network Qml Concurrent Test Xml MODULE)
find_package(Threads)

add_subdirectory(tests)
add_subdirectory(tools)
