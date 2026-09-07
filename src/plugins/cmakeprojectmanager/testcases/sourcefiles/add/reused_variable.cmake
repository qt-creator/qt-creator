cmake_minimum_required(VERSION 3.16)

project(HelloQt VERSION 0.1 LANGUAGES CXX)

set(SOURCES
    app.cpp
)

add_executable(HelloQt ${SOURCES})

set(SOURCES
    lib.cpp
)

add_library(HelloLib ${SOURCES})
