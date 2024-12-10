cmake_minimum_required(VERSION 3.16)

project(QuickQanava VERSION 2.5.0 LANGUAGES CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick Qml Quick QuickControls2)

message("Building QuickQanava for Qt${QT_VERSION_MAJOR}")

add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x050F00)

add_subdirectory(src)
