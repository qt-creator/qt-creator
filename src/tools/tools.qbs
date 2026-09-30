import qbs
import qbs.File

Project {
    name: "Tools"
    references: [
        "buildoutputparser/buildoutputparser.qbs",
        "cplusplustools.qbs",
        "disclaim/disclaim.qbs",
        "etwcapture/etwcapture.qbs",
        "process_stub/process_stub.qbs",
        "qmlpuppet/qmlpuppet.qbs",
        "qtcdebugger/qtcdebugger.qbs",
        "qtcreatorcrashhandler/qtcreatorcrashhandler.qbs",
        "qtc-askpass/qtc-askpass.qbs",
        "qtpromaker/qtpromaker.qbs",
        "sdktool/sdktool.qbs",
        "sdktool/sdktoollib.qbs",
        "valgrindfake/valgrindfake.qbs",
        "iostool/iostool.qbs",
        "qtprofiler/qtprofiler.qbs",
    ].concat(project.additionalTools)
}
