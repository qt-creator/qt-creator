cmake_minimum_required(VERSION 3.16)
project(cdbtypes CXX)

add_library(problib SHARED lib.cpp)

add_executable(app app.cpp)
