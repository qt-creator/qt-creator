add_qtc_plugin(mcpserver
  CONDITION TARGET McpServerLib
  PLUGIN_CLASS McpServerPlugin
  PLUGIN_DEPENDS
    Core
    ProjectExplorer
  DEPENDS
   ExtensionSystem
   Utils
   McpServerLib
  SOURCES
    issuesmanager.cpp issuesmanager.h
    mcpcommands.cpp mcpcommands.h
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
    schemas/issues-schema.json
    schemas/search-results-schema.json
)
