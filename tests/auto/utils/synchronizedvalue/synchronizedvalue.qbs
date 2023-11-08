import qbs

QtcAutotest {
    name: "synchronizedvalue autotest"
    Depends { name: "Utils" }
    files: "tst_synchronizedvalue.cpp"
}
