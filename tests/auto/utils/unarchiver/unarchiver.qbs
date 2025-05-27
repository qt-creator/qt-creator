import qbs

QtcAutotest {
    name: "Unarchiver autotest"
    Depends { name: "qtcLibArchive" }
    Depends { name: "Utils" }
    files: "tst_unarchiver.cpp"
}
