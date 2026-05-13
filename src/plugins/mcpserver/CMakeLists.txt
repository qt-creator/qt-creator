add_qtc_plugin(mcpserver
  CONDITION TARGET McpServerLib
  PLUGIN_CLASS McpServerPlugin
  PLUGIN_DEPENDS
    Core
    TextEditor
    ProjectExplorer
  DEPENDS
   ExtensionSystem
   Utils
   McpServerLib
  LONG_DESCRIPTION_MD McpServerDescription.md
  SOURCES
    mcpcommands.cpp mcpcommands.h
    mcpserverinspector.cpp mcpserverinspector.h
    mcpserverplugin.cpp
    mcpserverconstants.h
    mcpservertr.h
  SBOM_ARGS
    USE_ATTRIBUTION_FILES
    CREATE_SBOM_FOR_EACH_ATTRIBUTION
    ATTRIBUTION_FILE_PATHS
      "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS
      davecotter-mcp
)

qtc_add_resources(mcpserver "images"
  PREFIX "/mcpserver"
  BASE "."
  FILES
    images/mcpicon.png
    images/mcpicon@2x.png
)

qtc_add_resources(mcpserver "schemas"
  PREFIX "/mcpserver"
  BASE "."
  FILES
    schemas/search-results-schema.json
)
