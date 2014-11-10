import qbs

QtcAutotest {
    name: "FileUtils autotest"
    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" } // TODO: Remove when qbs bug is fixed
    Properties {
        condition: qbs.toolchain.contains("gcc")
        cpp.cxxFlags: base.concat(["-Wno-trigraphs"])
    }
    files: "tst_fileutils.cpp"
}
