cmake_minimum_required(VERSION 3.16)

project(HelloQt VERSION 0.1 LANGUAGES CXX)

include(QtTracing.cmake)

add_executable(HelloQt
    main.cpp
)

qt_add_tracepoints(HelloQt hello.tracepoints)

qt_add_tracepoints(HelloQt other.tracepoints)
