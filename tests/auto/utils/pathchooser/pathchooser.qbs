import qbs

QtcAutotest {
    name: "PathChooser autotest"
    Depends { name: "Utils" }
    files: "tst_pathchooser.cpp"
}
