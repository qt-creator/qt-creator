import qbs

QtcAutotest {
    name: "sdktool autotest"

    Group {
        name: "Test sources"
        files: "tst_sdktool.cpp"
    }
}
