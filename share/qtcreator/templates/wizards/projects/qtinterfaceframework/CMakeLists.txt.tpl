cmake_minimum_required(VERSION 3.16)
project(%{ProjectNameLowerCase} LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS InterfaceFramework Quick)

qt_standard_project_setup(REQUIRES 6.8)


## Application ##

qt_add_executable(%{ProjectNameLowerCase} WIN32
    main.cpp
)

qt_add_qml_module(%{ProjectNameLowerCase}
    URI %{ProjectNameCap}
    QML_FILES Main.qml
)

set_property(TARGET %{ProjectNameLowerCase} APPEND PROPERTY QT_ANDROID_EXTRA_PLUGINS
    "${CMAKE_CURRENT_BINARY_DIR}/interfaceframework"
)

target_link_libraries(%{ProjectNameLowerCase} PRIVATE
    %{ProjectNameLowerCase}_frontend
    Qt::Quick
)


## Frontend ##

qt_ifcodegen_add_qml_module(%{ProjectNameLowerCase}_frontend
    IDL_FILES %{ProjectNameLowerCase}.qface
    TEMPLATE frontend
)


## Backends
@if %{Simulator}
add_subdirectory(simulation_backend)
@endif
@if %{Backend}
add_subdirectory(production_backend)
@endif
@if %{QtRo}


## Remote Objects Backend ##

qt_ifcodegen_add_plugin(%{ProjectNameLowerCase}_qtro_backend
    IDL_FILES %{ProjectNameLowerCase}.qface
    TEMPLATE backend_qtro
)

set_target_properties(%{ProjectNameLowerCase}_qtro_backend PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY interfaceframework
)

target_link_libraries(%{ProjectNameLowerCase}_qtro_backend PRIVATE
    %{ProjectNameLowerCase}_frontend
)


## Remote Objects Servers ##
@if %{QtRoSimulation}
add_subdirectory(simulation_server)
@endif
@if %{QtRoProduction}
add_subdirectory(production_server)
@endif
@endif
