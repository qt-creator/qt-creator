cmake_minimum_required(VERSION 3.16)

project(HelloCpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Concurrent)

add_executable(HelloCpp main.cpp)

target_link_libraries(HelloCpp PRIVATE Qt6::Concurrent)

include(GNUInstallDirs)
install(TARGETS HelloCpp
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
