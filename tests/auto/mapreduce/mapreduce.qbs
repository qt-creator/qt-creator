import qbs

QtcAutotest {
    name: "Map reduce autotest"
    Depends { name: "Utils" }

    files: [
        "tst_mapreduce.cpp",
    ]
}
