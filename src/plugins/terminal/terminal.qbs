import qbs 1.0

QtcPlugin {
    name: "Terminal"

    Depends { name: "Core" }
    Depends { name: "vterm" }
    Depends { name: "ptyqt" }

    files: [
        "celllayout.cpp",
        "celllayout.h",
        "keys.cpp",
        "keys.h",
        "scrollback.cpp",
        "scrollback.h",
        "terminal.qrc",
        "terminalpane.cpp",
        "terminalpane.h",
        "terminalplugin.cpp",
        "terminalplugin.h",
        "terminalprocessinterface.cpp",
        "terminalprocessinterface.h",
        "terminalsettings.cpp",
        "terminalsettings.h",
        "terminalsettingspage.cpp",
        "terminalsettingspage.h",
        "terminaltr.h",
        "terminalwidget.cpp",
        "terminalwidget.h",
    ]
}

