Project {
    name: "QtcManualTests"

    qbsSearchPaths: "qbs"

    references: [
        "cmdbridge/cmdbridge.qbs",
        "debugger/gui/gui.qbs",
        "debugger/simple/simple.qbs",
        "devcontainer/devcontainer.qbs",
        "deviceshell/deviceshell.qbs",
        "fakevim/fakevim.qbs",
        "pluginview/pluginview.qbs",
        "proparser/testreader.qbs",
        "remotelinux/remotelinux.qbs",
        "scripts/scripts.qbs",
        "shootout/shootout.qbs",
        "spinner/spinner.qbs",
        "subdirfilecontainer/subdirfilecontainer.qbs",
        "terminal/terminal.qbs",
        "widgets/widgets.qbs",
    ]
}
