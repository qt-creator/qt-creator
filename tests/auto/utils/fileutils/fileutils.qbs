import qbs

QtcAutotest {
    name: "FileUtils autotest"
    Depends { name: "Utils" }
    files: "tst_fileutils.cpp"
}
