import qbs

Project {
    name: "Tools"
    references: [
        "qtcdebugger/qtcdebugger.qbs",
        "qtcreatorcrashhandler/qtcreatorcrashhandler.qbs",
        "qtpromaker/qtpromaker.qbs",
        "sdktool/sdktool.qbs"
    ]
}
