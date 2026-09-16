cmake_minimum_required(VERSION 3.16)

project(HelloQt VERSION 0.1 LANGUAGES CXX)

qt_add_executable(HelloQt
    main.cpp
    renderarea.cpp renderarea.h
    window.cpp window.h
    newwidget.cpp
)
