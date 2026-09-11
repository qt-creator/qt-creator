import qbs

QtcAutotest {
    name: "Sixel autotest"

    Depends { name: "Qt.gui" }
    Depends { name: "TerminalLib" }

    files: [
        "tst_sixel.cpp",
    ]
}
