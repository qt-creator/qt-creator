import qbs

Project {
    name: "QtcManualtests"

    condition: project.withAutotests

    references: [
        "debugger/gui/gui.qbs",
        "debugger/simple/simple.qbs",
        "deviceshell/deviceshell.qbs",
        "fakevim/fakevim.qbs",
        "pluginview/pluginview.qbs",
        "proparser/testreader.qbs",
        "shootout/shootout.qbs",
        "spinner/spinner.qbs",
        "subdirfileiterator/subdirfileiterator.qbs",
        "tasking/demo/demo.qbs",
        "tasking/imagescaling/imagescaling.qbs",
        "widgets/widgets.qbs",
    ]
}
