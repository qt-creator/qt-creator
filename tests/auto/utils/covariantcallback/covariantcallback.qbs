import qbs

QtcAutotest {
    name: "CovariantCallback autotest"
    Depends { name: "Utils" }
    files: "tst_covariantcallback.cpp"
}
