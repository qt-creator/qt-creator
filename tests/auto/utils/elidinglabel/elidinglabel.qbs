import qbs

QtcAutotest {
    name: "ElidingLabel autotest"
    Depends { name: "Utils" }
    files: "tst_elidinglabel.cpp"
}
