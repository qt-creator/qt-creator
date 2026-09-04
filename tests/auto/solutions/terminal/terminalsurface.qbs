import qbs

QtcAutotest {
    name: "TerminalSurface autotest"

    Depends { name: "Qt.gui" }
    Depends { name: "TerminalLib" }

    files: [
        "tst_terminalsurface.cpp",
    ]
}
