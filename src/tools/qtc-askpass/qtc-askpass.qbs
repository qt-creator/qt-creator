import qbs

QtcTool {
    name: "qtc-askpass"
    Depends { name: "Qt.widgets" }
    files: [
        "qtc-askpass-main.cpp",
    ]
}
