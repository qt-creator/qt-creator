import qbs 1.0

QtcPlugin {
    name: "Terminal"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TerminalLib" }

    files: [
        "shellmodel.cpp",
        "shellmodel.h",
        "shellintegration.cpp",
        "shellintegration.h",
        "shortcutmap.cpp",
        "shortcutmap.h",
        "terminal.qrc",
        "terminalconstants.h",
        "terminalicons.h",
        "terminalpane.cpp",
        "terminalpane.h",
        "terminalplugin.cpp",
        "terminalprocessimpl.cpp",
        "terminalprocessimpl.h",
        "terminalsettings.cpp",
        "terminalsettings.h",
        "terminaltr.h",
        "terminalwidget.cpp",
        "terminalwidget.h",
    ]
}

