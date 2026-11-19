import qbs

QtcAutotest {
    name: "Debugger UVSC autotest"
    Depends { name: "Debugger" }
    Depends { name: "Utils" }
    files: "tst_uvsc.cpp"
}
