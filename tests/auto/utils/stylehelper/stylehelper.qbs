import qbs

QtcAutotest {
    name: "StyleHelper autotest"
    Depends { name: "Utils" }
    files: "tst_stylehelper.cpp"
}
