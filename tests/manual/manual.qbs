import qbs

Project {
    name: "QtcManualtests"

    condition: project.withAutotests

    references: [
        "debugger/gui/gui.qbs",
        "debugger/simple/simple.qbs",
        "fakevim/fakevim.qbs",
        "pluginview/pluginview.qbs",
        "process/process.qbs",
        "proparser/testreader.qbs",
        "shootout/shootout.qbs",
        "widgets/widgets.qbs",
    ]
}
