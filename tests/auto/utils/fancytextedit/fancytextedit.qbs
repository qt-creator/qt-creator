import qbs

QtcAutotest {
    name: "FancyTextEdit autotest"
    Depends { name: "Utils" }
    files: "tst_fancytextedit.cpp"
}
