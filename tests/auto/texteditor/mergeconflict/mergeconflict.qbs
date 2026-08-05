import qbs

QtcAutotest {

    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" }

    name: "MergeConflict autotest"

    Group {
        name: "Source Files"
        files: "tst_mergeconflict.cpp"
    }
}
