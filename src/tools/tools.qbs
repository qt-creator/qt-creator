import qbs

Project {
    name: "Tools"
    references: [
        "buildoutputparser/buildoutputparser.qbs",
        "clangbackend/clangbackend.qbs",
        "clangpchmanagerbackend/clangpchmanagerbackend.qbs",
        "clangrefactoringbackend/clangrefactoringbackend.qbs",
        "cplusplustools.qbs",
        "qml2puppet/qml2puppet.qbs",
        "qtcdebugger/qtcdebugger.qbs",
        "qtcreatorcrashhandler/qtcreatorcrashhandler.qbs",
        "qtpromaker/qtpromaker.qbs",
        "sdktool/sdktool.qbs",
        "valgrindfake/valgrindfake.qbs",
        "iostool/iostool.qbs",
        "winrtdebughelper/winrtdebughelper.qbs"
    ].concat(project.additionalTools)
}
