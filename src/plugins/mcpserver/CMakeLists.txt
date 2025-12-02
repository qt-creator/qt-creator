find_package(Qt6 OPTIONAL_COMPONENTS HttpServer QUIET)

add_qtc_plugin(mcpserver
  CONDITION TARGET Qt::HttpServer
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

qtc_add_resources(mcpserver "images"
  PREFIX "/mcpserver"
  BASE "."
  FILES
    images/mcpicon.png
    images/mcpicon@2x.png
)
