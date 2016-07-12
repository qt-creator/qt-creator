import qbs

QtcAutotest {
    name: "Algorithm autotest"
    Depends { name: "Utils" }

    files: [
        "tst_algorithm.cpp",
    ]
}
