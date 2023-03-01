
add_qtc_plugin(Terminal
  PLUGIN_DEPENDS Core
  DEPENDS libvterm
  SOURCES
    celllayout.cpp celllayout.h
    terminal.qrc
    terminalplugin.cpp terminalplugin.h
    terminaltr.h
    terminalpane.cpp terminalpane.h
    terminalwidget.cpp terminalwidget.h
    terminalprocessinterface.cpp terminalprocessinterface.h
    terminalsettings.cpp terminalsettings.h
    terminalsettingspage.cpp terminalsettingspage.h
    scrollback.h scrollback.cpp
    shellmodel.cpp shellmodel.h
    keys.cpp keys.h
)
