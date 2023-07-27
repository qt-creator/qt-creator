import qbs 1.0

QtcLibrary {
    name: "TerminalLib"

    Depends { name: "vterm" }
    Depends { name: "Qt"; submodules: "widgets" }

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
        "terminal_global.h",
        "terminalsurface.cpp",
        "terminalsurface.h",
        "terminalview.cpp",
        "terminalview.h",
    ]
}
