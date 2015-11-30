import qbs

QtcAutotest {
    Depends { name: "CppTools" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" } // For QTextDocument & friends
}
