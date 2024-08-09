qt_add_executable(%{ProjectNameLowerCase}_qtro_simulation_server WIN32)

qt_ifcodegen_extend_target(%{ProjectNameLowerCase}_qtro_simulation_server
    IDL_FILES ../%{ProjectNameLowerCase}.qface
    TEMPLATE server_qtro_simulator
)

target_link_libraries(%{ProjectNameLowerCase}_qtro_simulation_server PRIVATE
    %{ProjectNameLowerCase}_frontend
)

qt_add_resources(%{ProjectNameLowerCase}_qtro_simulation_server "plugin_resource"
    PREFIX
        "/plugin_resource"
    FILES
         "simulation.qml"
)
