import qbs

QtcAutotest {
    name: "TerminalView autotest"

    Depends { name: "Qt.widgets" }
    Depends { name: "TerminalLib" }

    files: [
        "tst_terminalview.cpp",
    ]
}
