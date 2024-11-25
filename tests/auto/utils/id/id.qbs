import qbs

QtcAutotest {
    name: "Id autotest"
    Depends { name: "Utils" }
    files: "tst_id.cpp"
}
