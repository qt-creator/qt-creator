import qbs

QtcAutotest {
    name: "UnixDeviceFileAccess autotest"
    Depends { name: "Utils" }
    Properties {
        condition: qbs.toolchain.contains("gcc")
        cpp.cxxFlags: base.concat(["-Wno-trigraphs"])
    }
    files: "tst_unixdevicefileaccess.cpp"
}
