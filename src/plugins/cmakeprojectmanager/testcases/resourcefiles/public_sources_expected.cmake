cmake_minimum_required(VERSION 3.16)

project(HelloQt VERSION 0.1 LANGUAGES CXX)

qt_add_library(HelloQt STATIC)

set_target_properties(HelloQt PROPERTIES AUTORCC ON)

target_sources(HelloQt
    PUBLIC
        api.cpp
    PRIVATE
        assets.qrc
)

target_link_libraries(HelloQt PRIVATE Qt6::Core)
