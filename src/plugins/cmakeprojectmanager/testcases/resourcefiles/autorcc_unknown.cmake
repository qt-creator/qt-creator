cmake_minimum_required(VERSION 3.16)

project(HelloQt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTORCC ${ENABLE_RESOURCES})

qt_add_executable(HelloQt
    main.cpp
)

target_link_libraries(HelloQt PRIVATE Qt6::Core)
