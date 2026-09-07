cmake_minimum_required(VERSION 3.16)

project(HelloQt VERSION 0.1 LANGUAGES CXX)

set(SOURCES main.cpp CACHE INTERNAL "The sources of the application")

add_executable(HelloQt
    ${SOURCES}
)
