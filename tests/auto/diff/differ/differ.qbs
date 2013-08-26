import qbs
import "../../autotest.qbs" as Autotest

Autotest {
    name: "Differ autotest"
    Depends { name: "DiffEditor" }
    Depends { name: "Qt.widgets" } // For QTextDocument
    files: "tst_differ.cpp"
}
