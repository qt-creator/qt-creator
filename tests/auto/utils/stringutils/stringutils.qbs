import qbs

QtcAutotest {
    name: "StringUtils autotest"
    Depends { name: "Utils" }
    files: "tst_stringutils.cpp"
}
