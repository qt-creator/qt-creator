cmake_minimum_required(VERSION 3.16)

project(HelloQt VERSION 0.1 LANGUAGES CXX)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        newwidget.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(HelloQt
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(HelloQt
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(HelloQt PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
