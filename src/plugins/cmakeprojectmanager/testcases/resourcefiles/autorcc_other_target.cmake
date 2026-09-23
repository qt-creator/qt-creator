cmake_minimum_required(VERSION 3.16)

project(HelloQt VERSION 0.1 LANGUAGES CXX)

qt_add_executable(HelloQt
    main.cpp
)

qt_add_executable(HelloOther
    other.cpp
)

set_target_properties(HelloOther PROPERTIES OUTPUT_NAME HelloQt AUTORCC ON)
