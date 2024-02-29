cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 14)
project(conan-libfmt)

find_package(fmt REQUIRED)

add_executable(conan-libfmt main.cpp)
target_link_libraries(conan-libfmt PRIVATE fmt::fmt)

install(TARGETS conan-libfmt)
