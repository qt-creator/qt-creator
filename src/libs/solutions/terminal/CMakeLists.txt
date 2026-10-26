add_qtc_library(TerminalLib
  DEPENDS Qt::Core Qt::Widgets libvterm
  SOURCES
    boxdrawing.cpp boxdrawing.h
    celliterator.cpp celliterator.h
    glyphcache.cpp glyphcache.h
    keys.cpp keys.h
    scrollback.cpp scrollback.h
    sixel.cpp sixel.h
    surfaceintegration.h
    terminal_global.h
    terminalsurface.cpp terminalsurface.h
    terminalview.cpp terminalview.h
)

qtc_add_resources(TerminalLib "images"
  PREFIX "/terminal"
  FILES images/passwordlock.png
)
