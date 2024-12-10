cmake_minimum_required(VERSION 3.1.0)

project(QuickContainers LANGUAGES CXX)

# Configure Qt
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Quick REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Qml REQUIRED)
set(CMAKE_AUTOMOC ON)

set (qcm_source_files
    ${CMAKE_CURRENT_SOURCE_DIR}/src/qcmAbstractContainer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/qcmContainerModel.cpp
)

set (qcm_header_files
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qcmAbstractContainer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qcmAdapter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qcmContainer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/qcmContainerModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/QuickContainers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/QuickContainers
)

set (qcm_plugin_files
    ${CMAKE_CURRENT_SOURCE_DIR}/src/qcmPlugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/qcmPlugin.h
)

# Configure QuickContainers library ###########################################
add_library(QuickContainers STATIC ${qcm_header_files} ${qcm_source_files})
set_target_properties(QuickContainers PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(QuickContainers
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/quickcontainers>
    )

target_compile_features(QuickContainers
    PUBLIC
        cxx_std_14
    )

