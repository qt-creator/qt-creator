import qbs

QtcAutotest {
    name: "FilePath autotest"
    Depends { name: "Utils" }
    files: "tst_filepath.cpp"
}
