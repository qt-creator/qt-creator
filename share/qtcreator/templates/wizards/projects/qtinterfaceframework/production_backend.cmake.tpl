qt_add_plugin(%{ProjectNameLowerCase}_production_backend
    CLASS_NAME %{ProjectNameCap}ModuleBackend
    PLUGIN_TYPE "interfaceframework"
    %{ProjectNameLowerCase}plugin.h
    %{ProjectNameLowerCase}plugin.cpp
    %{FeatureLowerCase}backend.h
    %{FeatureLowerCase}backend.cpp
)

set_target_properties(%{ProjectNameLowerCase}_production_backend PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ../interfaceframework
)

target_link_libraries(%{ProjectNameLowerCase}_production_backend PRIVATE
    %{ProjectNameLowerCase}_frontend
    Qt6::InterfaceFramework
)

target_include_directories(%{ProjectNameLowerCase}_production_backend PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
