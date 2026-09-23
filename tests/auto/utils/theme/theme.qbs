import qbs

QtcAutotest {
    name: "Theme autotest"
    Depends { name: "Utils" }
    files: "tst_theme.cpp"
}
