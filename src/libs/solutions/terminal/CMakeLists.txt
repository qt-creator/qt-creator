add_qtc_library(TerminalLib
  DEPENDS Qt::Core Qt::Widgets libvterm
  SOURCES
    celliterator.cpp celliterator.h
    glyphcache.cpp glyphcache.h
    keys.cpp keys.h
    scrollback.cpp scrollback.h
    surfaceintegration.h
    terminal_global.h
    terminal.qrc
    terminalsurface.cpp terminalsurface.h
    terminalview.cpp terminalview.h
)
