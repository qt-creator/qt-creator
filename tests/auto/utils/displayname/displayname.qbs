import qbs

QtcAutotest {
    name: "DisplayName autotest"
    Depends { name: "Utils" }
    files: "tst_displayname.cpp"
}
