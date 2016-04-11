import qbs

QtcAutotest {
    name: "offsets autotest"
    Depends { name: "Qt.core-private" }
    Group {
        name: "Test sources"
        files: "tst_offsets.cpp"
    }
}
