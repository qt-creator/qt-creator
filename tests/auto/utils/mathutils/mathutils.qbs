import qbs

QtcAutotest {
    name: "MathUtils autotest"
    Depends { name: "Utils" }
    files: "tst_mathutils.cpp"
}
