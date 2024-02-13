import qbs

QtcAutotest {
    name: "UnixDeviceFileAccess autotest"
    Depends { name: "Utils" }
    files: "tst_unixdevicefileaccess.cpp"
}
