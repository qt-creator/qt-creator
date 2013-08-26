import qbs
import "../autotest.qbs" as Autotest

Autotest {
    Depends { name: "CppTools" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" } // For QTextDocument & friends
}
