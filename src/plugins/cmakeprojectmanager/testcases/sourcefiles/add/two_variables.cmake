cmake_minimum_required(VERSION 3.16)

project(HelloQt VERSION 0.1 LANGUAGES CXX)

set(GUI_SOURCES
    mainwindow.cpp
)

set(APP_SOURCES
    main.cpp
)

add_executable(HelloQt
    ${GUI_SOURCES}
    ${APP_SOURCES}
)
