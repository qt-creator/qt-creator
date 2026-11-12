import qbs

QtcAutotest {
    name: "Aspects autotest"
    Depends { name: "Utils" }
    files: "tst_aspects.cpp"
}
