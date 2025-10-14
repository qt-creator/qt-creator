cmake_minimum_required(VERSION 3.16)

project(HelloQt LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Concurrent)

add_executable(HelloQt main.cpp)

target_link_libraries(HelloQt PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(HelloQt PRIVATE Qt${QT_VERSION_MAJOR}::Concurrent)

include(GNUInstallDirs)
install(TARGETS HelloQt
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
