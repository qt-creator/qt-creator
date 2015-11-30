import qbs

QtcAutotest {
    name: "ANSI autotest"
    Depends { name: "Utils" }
    files: "tst_ansiescapecodehandler.cpp"
}
