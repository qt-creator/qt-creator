import qbs

QtcAutotest {
    name: "TreeViewFind autotest"
    Depends { name: "Core" }
    Depends { name: "Qt.widgets" } // For QTextDocument
    files: "tst_treeviewfind.cpp"
}
