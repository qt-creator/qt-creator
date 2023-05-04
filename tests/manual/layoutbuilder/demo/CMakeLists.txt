cmake_minimum_required(VERSION 3.5)

project(layoutbuilderdemo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        ../../../../src/libs/utils/layoutbuilder.h
        ../../../../src/libs/utils/layoutbuilder.cpp
)

add_executable(layoutbuilderdemo
    ${PROJECT_SOURCES}
)

target_include_directories(layoutbuilderdemo PRIVATE
    ../../../../src/libs/utils/
)

target_link_libraries(layoutbuilderdemo PRIVATE
    Qt6::Widgets
)

set_target_properties(layoutbuilderdemo PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS layoutbuilderdemo
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
