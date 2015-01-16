import qbs

QtcAutotest {
    name: "StringUtils autotest"
    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" } // TODO: Remove when qbs bug is fixed
    files: "tst_stringutils.cpp"
}
