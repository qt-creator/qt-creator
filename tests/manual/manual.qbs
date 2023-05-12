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
        "subdirfileiterator/subdirfileiterator.qbs",
        "tasktree/tasktree.qbs",
        "widgets/widgets.qbs",
    ]
}
