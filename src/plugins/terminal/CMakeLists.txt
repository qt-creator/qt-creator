
add_qtc_plugin(Terminal
  PLUGIN_DEPENDS Core ProjectExplorer
  DEPENDS libvterm
  SOURCES
    celliterator.cpp celliterator.h
    glyphcache.cpp glyphcache.h
    keys.cpp keys.h
    scrollback.cpp scrollback.h
    shellintegration.cpp shellintegration.h
    shellmodel.cpp shellmodel.h
    shortcutmap.cpp shortcutmap.h
    terminal.qrc
    terminalconstants.h
    terminalicons.h
    terminalpane.cpp terminalpane.h
    terminalplugin.cpp
    terminalprocessimpl.cpp terminalprocessimpl.h
    terminalsearch.cpp terminalsearch.h
    terminalsettings.cpp terminalsettings.h
    terminalsurface.cpp terminalsurface.h
    terminaltr.h
    terminalwidget.cpp terminalwidget.h
)
