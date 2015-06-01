import qbs

Project {
    name: "Tools"
    references: [
        "buildoutputparser/buildoutputparser.qbs",
        "codemodelbackend/codemodelbackend.qbs",
        "cplusplustools.qbs",
        "qtcdebugger/qtcdebugger.qbs",
        "qtcreatorcrashhandler/qtcreatorcrashhandler.qbs",
        "qtpromaker/qtpromaker.qbs",
        "sdktool/sdktool.qbs",
        "valgrindfake/valgrindfake.qbs",
        "3rdparty/iossim/iossim.qbs",
        "iostool/iostool.qbs",
        "winrtdebughelper/winrtdebughelper.qbs"
    ].concat(project.additionalTools)
}
