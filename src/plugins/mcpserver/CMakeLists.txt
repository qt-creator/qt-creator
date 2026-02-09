add_qtc_plugin(mcpserver
  PLUGIN_DEPENDS
    QtCreator::Core
    QtCreator::ProjectExplorer
  DEPENDS
   Qt::Widgets
   Qt::Network
   QtCreator::ExtensionSystem
   QtCreator::Utils
  SOURCES
    httpparser.cpp httpparser.h
    httpresponse.cpp httpresponse.h
    issuesmanager.cpp issuesmanager.h
    mcpserver.cpp mcpserver.h
    mcpservertest.cpp mcpservertest.h
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
    issues-schema.json
)
