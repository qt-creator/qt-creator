import qbs

QtcAutotest {
    name: "StyledBar autotest"
    Depends { name: "Utils" }
    files: "tst_styledbar.cpp"
}
