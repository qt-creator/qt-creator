import qbs

QtcAutotest {
    name: "MergeModel autotest"
    Depends { name: "Utils" }
    Depends { name: "DiffEditor" }
    Depends { name: "Qt.widgets" } // For QTextDocument
    files: "tst_mergemodel.cpp"
}
