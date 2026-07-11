import qbs

QtcAutotest {
    name: "InlineDiff autotest"
    Depends { name: "Utils" }
    Depends { name: "DiffEditor" }
    Depends { name: "Qt.widgets" }
    files: "tst_inlinediff.cpp"
}
