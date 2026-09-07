cmake_minimum_required(VERSION 3.16)

project(HelloQt VERSION 0.1 LANGUAGES CXX)

set(SOURCES
    main.cpp
    newwidget.cpp
)

my_add_app(HelloQt)

target_sources(HelloQt
    PRIVATE
        ${SOURCES}
)
