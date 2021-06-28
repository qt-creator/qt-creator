import qbs

QtcAutotest {
    name: "MultiTextCursor autotest"
    Depends { name: "Utils" }
    files: "tst_multicursor.cpp"
}
