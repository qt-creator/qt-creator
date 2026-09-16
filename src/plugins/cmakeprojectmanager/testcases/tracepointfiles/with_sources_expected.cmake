cmake_minimum_required(VERSION 3.16)

project(HelloQt VERSION 0.1 LANGUAGES CXX)

qt_add_executable(HelloQt
    main.cpp
    renderarea.cpp renderarea.h
    window.cpp window.h
    hello_tracing.h
)

include(QtTracing.cmake)
qt_add_tracepoints(HelloQt hello.tracepoints)

target_link_libraries(HelloQt PRIVATE Qt6::Core)
