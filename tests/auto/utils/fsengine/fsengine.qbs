import qbs

QtcAutotest {
    name: "FSEngine autotest"
    Depends { name: "Utils" }
    files: "tst_fsengine.cpp"
}
