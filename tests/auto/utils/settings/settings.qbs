import qbs

QtcAutotest {
    name: "Settings autotest"
    Depends { name: "Utils" }
    files: "tst_settings.cpp"
}
