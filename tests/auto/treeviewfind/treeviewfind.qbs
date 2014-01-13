import qbs
import "../autotest.qbs" as Autotest

Autotest {
    name: "TreeViewFind autotest"
    Depends { name: "Core" }
    Depends { name: "Qt.widgets" } // For QTextDocument
    files: "tst_treeviewfind.cpp"
}
