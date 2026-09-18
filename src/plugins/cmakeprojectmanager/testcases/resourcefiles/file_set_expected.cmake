cmake_minimum_required(VERSION 3.23)

project(HelloQt VERSION 0.1 LANGUAGES CXX)

qt_add_library(HelloQt STATIC)

set_target_properties(HelloQt PROPERTIES AUTORCC ON)

target_sources(HelloQt
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES include/helloqt/api.h
    PRIVATE
        assets.qrc
)

target_link_libraries(HelloQt PRIVATE Qt6::Core)
