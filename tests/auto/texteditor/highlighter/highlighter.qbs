import qbs

QtcAutotest {

    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" } // For QTextDocument & friends

    name: "Highlighter autotest"

    Group {
        name: "Source Files"
        files: "tst_highlighter.cpp"
    }
}
