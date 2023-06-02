import qbs

QtcAutotest {
    name: "Text autotest"
    Depends { name: "Utils" }
    files: "tst_text.cpp"
}
