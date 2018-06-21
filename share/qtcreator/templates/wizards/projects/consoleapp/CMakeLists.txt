cmake_minimum_required(VERSION 2.8.12)

project(%{ProjectName})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Core)

add_executable(${PROJECT_NAME} "%{CppFileName}")

target_link_libraries(${PROJECT_NAME} Qt5::Core)
