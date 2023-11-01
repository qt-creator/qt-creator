cmake_minimum_required(VERSION 3.16)
project(%{CMakeProjectName} LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)

# Define the condition for HOST_BUILD
set(HOST_BUILD OFF)  # Default to OFF

if (NOT CMAKE_CROSSCOMPILING AND (NOT UNIX OR NOT (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm|aarch64") OR APPLE))
    set(HOST_BUILD ON)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick SafeRenderer SafeRendererTools SafePlatformAdaptation)

set(sources main.cpp
)

set (safeqmls
"main.qml"
)

# Resource files are passed to qtsafelayouttool
set(resource_files
    "qml.qrc"
)

#resource.bin is loaded by qtsafelayouttool to find the resource data asset.
qt6_add_binary_resources(resources_%{CMakeProjectName} ${resource_files} DESTINATION resource.bin)
qsr_add_safelayout(generatelayout_%{CMakeProjectName} SAFE_QMLS ${safeqmls}
                                  OUTPUT_PATH ${CMAKE_CURRENT_LIST_DIR}/layoutData
                                  SAFE_RESOURCE "${CMAKE_CURRENT_LIST_DIR}/safeasset.qrc"
                                  INPUT_RESOURCES resource.bin)
qsr_add_resource(buildresource_%{CMakeProjectName} sources "${CMAKE_CURRENT_LIST_DIR}/safeasset.qrc")

if (HOST_BUILD)
    qt6_add_resources(sources ${resource_files})
endif()

add_executable(%{CMakeProjectName} WIN32 MACOSX_BUNDLE
    ${sources}
)

#Enable when using monitor feature:
#target_compile_definitions(%{CMakeProjectName} PRIVATE USE_OUTPUTVERIFIER)

add_dependencies(%{CMakeProjectName} generatelayout_%{CMakeProjectName})

if (HOST_BUILD)
    target_compile_definitions(%{CMakeProjectName} PUBLIC
        HOST_BUILD
    )

    target_link_libraries(%{CMakeProjectName} PUBLIC
        Qt::Quick
        Qt::Widgets
        Qt::Qml
    )
else()
    message(STATUS "Project is not linked with Qt when building for embedded systems.")
endif()

target_link_libraries(%{CMakeProjectName} PUBLIC
    Qt::SafeRenderer
    Qt::SafePlatformAdaptation
)
