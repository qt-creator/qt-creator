qt_add_executable(%{ProjectNameLowerCase}_qtro_production_server WIN32
    %{FeatureLowerCase}backend.h
    %{FeatureLowerCase}backend.cpp
    %{FeatureLowerCase}adapter.h
    %{FeatureLowerCase}adapter.cpp
    servermain.h
    servermain.cpp
)

qt_ifcodegen_extend_target(%{ProjectNameLowerCase}_qtro_production_server
    IDL_FILES ../%{ProjectNameLowerCase}.qface
    TEMPLATE server_qtro
)

target_link_libraries(%{ProjectNameLowerCase}_qtro_production_server PRIVATE
    %{ProjectNameLowerCase}_frontend
    Qt6::InterfaceFramework
)

target_include_directories(%{ProjectNameLowerCase}_qtro_production_server PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
