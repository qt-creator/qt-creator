qt_ifcodegen_add_plugin(%{ProjectNameLowerCase}_simulation_backend
    IDL_FILES ../%{ProjectNameLowerCase}.qface
    TEMPLATE backend_simulator
)

set_target_properties(%{ProjectNameLowerCase}_simulation_backend PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ../interfaceframework
)

target_link_libraries(%{ProjectNameLowerCase}_simulation_backend PRIVATE
    %{ProjectNameLowerCase}_frontend
)

qt_add_resources(%{ProjectNameLowerCase}_simulation_backend "plugin_resource"
    PREFIX
        "/plugin_resource"
    FILES
         "simulation.qml"
)
