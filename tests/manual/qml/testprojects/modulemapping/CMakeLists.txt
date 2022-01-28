cmake_minimum_required(VERSION 3.16)
project(test_project)

add_executable(test_exe test.cc test.qml)

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/qml_module_mappings/test_exe" CONTENT "QtQuick.Controls=MyControls\n")

