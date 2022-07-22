cmake_minimum_required(VERSION 3.15)

project(vcpkg-libfmt)

find_package(fmt CONFIG REQUIRED)

add_executable(vcpkg-libfmt main.cpp)
target_link_libraries(vcpkg-libfmt PRIVATE fmt::fmt)

install(TARGETS vcpkg-libfmt)
