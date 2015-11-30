import qbs

QtcAutotest {
    name: "FileUtils autotest"
    Depends { name: "Utils" }
    Properties {
        condition: qbs.toolchain.contains("gcc")
        cpp.cxxFlags: base.concat(["-Wno-trigraphs"])
    }
    files: "tst_fileutils.cpp"
}
