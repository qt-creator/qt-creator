cmake_minimum_required(VERSION 3.16)

project(HelloQt VERSION 0.1 LANGUAGES CXX)

qt_add_executable(HelloQt
    main.cpp
)

target_sources(HelloQt
    PRIVATE
        assets.qrc
)

set_property(TARGET HelloQt PROPERTY AUTORCC ON)

target_link_libraries(HelloQt PRIVATE Qt6::Core)
