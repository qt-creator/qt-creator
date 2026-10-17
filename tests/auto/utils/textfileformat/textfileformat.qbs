import qbs

QtcAutotest {
    name: "TextFileFormat autotest"
    Depends { name: "Utils" }
    files: "tst_textfileformat.cpp"
}
