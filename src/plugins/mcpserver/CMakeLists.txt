find_package(Qt6 COMPONENTS HttpServer QUIET)

add_qtc_plugin(mcpserver
  CONDITION Qt6HttpServer_FOUND
  PLUGIN_DEPENDS
    QtCreator::Core
    QtCreator::ProjectExplorer
  DEPENDS
   Qt::HttpServer
   Qt::Widgets
   Qt::Network
   QtCreator::ExtensionSystem
   QtCreator::Utils
  SOURCES
    issuesmanager.cpp issuesmanager.h
    mcp.webp
    mcp.qrc
    mcpserver.cpp mcpserver.h
    mcpservertest.cpp mcpservertest.h
    mcpcommands.cpp mcpcommands.h
    plugin.cpp
    pluginconstants.h
    plugintr.h
  SBOM_ARGS
    USE_ATTRIBUTION_FILES
    CREATE_SBOM_FOR_EACH_ATTRIBUTION
    ATTRIBUTION_FILE_PATHS
      "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS
      davecotter-mcp
)

# Generate the MCP discovery file with version substitution
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mcp_discovery.json.in
  ${CMAKE_CURRENT_BINARY_DIR}/mcp_discovery.json
  @ONLY
)
