
add_qtc_plugin(Terminal
  PLUGIN_DEPENDS Core ProjectExplorer
  DEPENDS TerminalLib
  SOURCES
    shellintegration.cpp shellintegration.h
    shellmodel.cpp shellmodel.h
    shortcutmap.cpp shortcutmap.h
    terminal.qrc
    terminalconstants.h
    terminalicons.h
    terminalpane.cpp terminalpane.h
    terminalplugin.cpp
    terminalprocessimpl.cpp terminalprocessimpl.h
    terminalsettings.cpp terminalsettings.h
    terminaltr.h
    terminalwidget.cpp terminalwidget.h
  SBOM_ARGS
    USE_ATTRIBUTION_FILES
    CREATE_SBOM_FOR_EACH_ATTRIBUTION
    ATTRIBUTION_FILE_PATHS
      "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS
      terminal-shellintegrations
      terminal-shellintegrations-clink
)
