import qbs

QtcAutotest {
    name: "FileSystemModel autotest"
    Depends { name: "Utils" }
    files: "tst_filesystemmodel.cpp"
}
