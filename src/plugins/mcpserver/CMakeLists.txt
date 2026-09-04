add_qtc_plugin(McpServer
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
    mcplogcapture.cpp mcplogcapture.h
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

extend_qtc_plugin(McpServer
  CONDITION WITH_TESTS
  SOURCES
    mcpcommands_test.cpp mcpcommands_test.h
)

qtc_add_resources(McpServer "images"
  PREFIX "/mcpserver"
  BASE "."
  FILES
    images/mcpicon.png
    images/mcpicon@2x.png
)

qtc_add_resources(McpServer "schemas"
  PREFIX "/mcpserver"
  BASE "."
  FILES
    schemas/search-results-schema.json
)
