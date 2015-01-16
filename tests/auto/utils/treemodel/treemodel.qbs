import qbs

QtcAutotest {
    name: "TreeModel autotest"
    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" } // TODO: Remove when qbs bug is fixed
    files: "tst_treemodel.cpp"
}
