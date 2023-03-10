
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
    terminal.qrc
    terminalpane.cpp terminalpane.h
    terminalplugin.cpp terminalplugin.h
    terminalprocessinterface.cpp terminalprocessinterface.h
    terminalsettings.cpp terminalsettings.h
    terminalsettingspage.cpp terminalsettingspage.h
    terminalsurface.cpp terminalsurface.h
    terminaltr.h
    terminalwidget.cpp terminalwidget.h
)
