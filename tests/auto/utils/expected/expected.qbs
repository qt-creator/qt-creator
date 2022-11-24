import qbs

QtcAutotest {
    name: "Expected autotest"
    Depends { name: "Utils" }
    files: "tst_expected.cpp"
}
