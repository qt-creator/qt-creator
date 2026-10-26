QtcLibrary {
    name: "TerminalLib"

    Depends { name: "vterm" }
    Depends { name: "Qt.widgets" }

    cpp.defines: base.concat("TERMINALLIB_LIBRARY")

    files: [
        "boxdrawing.cpp",
        "boxdrawing.h",
        "celliterator.cpp",
        "celliterator.h",
        "glyphcache.cpp",
        "glyphcache.h",
        "keys.cpp",
        "keys.h",
        "scrollback.cpp",
        "scrollback.h",
        "sixel.cpp",
        "sixel.h",
        "surfaceintegration.h",
        "terminal_global.h",
        "terminalsurface.cpp",
        "terminalsurface.h",
        "terminalview.cpp",
        "terminalview.h",
    ]

    Group {
        name: "images"
        Qt.core.resourcePrefix: "/terminal"
        Qt.core.resourceSourceBase: sourceDirectory
        files: "images/passwordlock.png"
        fileTags: "qt.core.resource_data"
    }
}
