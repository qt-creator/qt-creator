import qbs

QtcAutotest {
    name: "Run extensions autotest"
    Depends { name: "Utils" }

    files: [
        "tst_runextensions.cpp",
    ]
}
