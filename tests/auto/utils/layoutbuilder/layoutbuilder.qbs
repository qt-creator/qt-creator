import qbs

QtcAutotest {
    name: "LayoutBuilder autotest"
    Depends { name: "Utils" }
    files: "tst_layoutbuilder.cpp"
}
