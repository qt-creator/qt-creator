import qbs

QtcAutotest {
    name: "File search autotest"
    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" } // For QTextDocument
    files: [
        "testfile.txt",
        "tst_filesearch.cpp",
        "tst_filesearch.qrc"
    ]
}
