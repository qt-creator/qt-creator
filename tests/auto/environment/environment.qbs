import qbs

QtcAutotest {
    name: "Environment autotest"
    Depends { name: "Utils" }
    files: "tst_environment.cpp"
}
