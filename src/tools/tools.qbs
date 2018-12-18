import qbs
import qbs.File

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
        "qtc-askpass/qtc-askpass.qbs",
        "qtpromaker/qtpromaker.qbs",
        "sdktool/sdktool.qbs",
        "valgrindfake/valgrindfake.qbs",
        "iostool/iostool.qbs",
        "winrtdebughelper/winrtdebughelper.qbs"
    ].concat(project.additionalTools)

    Project {
        name: "PerfParser Tool"
        references: [
            "perfparser/perfparser.qbs"
        ]
        condition: File.exists(project.ide_source_tree + "/src/tools/perfparser/perfparser.qbs")
    }
}
