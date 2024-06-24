Project {
    name: "QtcManualTests"

    qbsSearchPaths: "qbs"

    references: [
        "cmdbridge/cmdbridge.qbs",
        "debugger/gui/gui.qbs",
        "debugger/simple/simple.qbs",
        "deviceshell/deviceshell.qbs",
        "fakevim/fakevim.qbs",
        "pluginview/pluginview.qbs",
        "proparser/testreader.qbs",
        "shootout/shootout.qbs",
        "spinner/spinner.qbs",
        "subdirfilecontainer/subdirfilecontainer.qbs",
        "tasking/assetdownloader/assetdownloader.qbs",
        "tasking/dataexchange/dataexchange.qbs",
        "tasking/demo/demo.qbs",
        "tasking/imagescaling/imagescaling.qbs",
        "tasking/trafficlight/trafficlight.qbs",
        "terminal/terminal.qbs",
        "widgets/widgets.qbs",
    ]
}
