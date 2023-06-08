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


set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(IMPLICIT_DEPENDS Qt::Test)

add_subdirectory(tests)
add_subdirectory(tools)
