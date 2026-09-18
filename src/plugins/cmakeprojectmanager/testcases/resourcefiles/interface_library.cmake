cmake_minimum_required(VERSION 3.16)

project(HelloQt VERSION 0.1 LANGUAGES CXX)

add_library(HelloQt INTERFACE)

set_target_properties(HelloQt PROPERTIES AUTORCC ON)

target_link_libraries(HelloQt INTERFACE Qt6::Core)
