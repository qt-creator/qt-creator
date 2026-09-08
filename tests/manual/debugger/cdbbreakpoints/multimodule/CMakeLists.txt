cmake_minimum_required(VERSION 3.16)
project(cdbmultimodule CXX)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(alpha SHARED alpha.cpp shared.cpp)
add_library(beta SHARED beta.cpp shared.cpp)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE alpha beta)
