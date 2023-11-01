QtcLibrary {
    name: "TerminalLib"

    Depends { name: "vterm" }
    Depends { name: "Qt.widgets" }

    cpp.defines: base.concat("TERMINALLIB_LIBRARY")

    files: [
        "celliterator.cpp",
        "celliterator.h",
        "glyphcache.cpp",
        "glyphcache.h",
        "keys.cpp",
        "keys.h",
        "scrollback.cpp",
        "scrollback.h",
        "surfaceintegration.h",
        "terminal.qrc",
        "terminal_global.h",
        "terminalsurface.cpp",
        "terminalsurface.h",
        "terminalview.cpp",
        "terminalview.h",
    ]
}
