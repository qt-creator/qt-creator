QtcAutotest {
    name: "UpdateInfo autotest"
    Depends { name: "Qt.xml" }
    Depends { name: "Utils" }
    files: "tst_updateinfo.cpp"
    cpp.includePaths: base.concat(["../../../src/plugins"])
}
