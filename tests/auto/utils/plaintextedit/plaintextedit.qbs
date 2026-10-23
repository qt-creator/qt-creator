import qbs

QtcAutotest {
    name: "PlainTextEdit autotest"
    Depends { name: "Utils" }
    files: "tst_plaintextedit.cpp"
}
