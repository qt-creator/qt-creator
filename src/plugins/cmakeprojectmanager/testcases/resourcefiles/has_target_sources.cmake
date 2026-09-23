cmake_minimum_required(VERSION 3.16)

project(HelloQt VERSION 0.1 LANGUAGES CXX)

qt_add_executable(HelloQt)

target_sources(HelloQt
    PRIVATE
        main.cpp
)

target_link_libraries(HelloQt PRIVATE Qt6::Core)
