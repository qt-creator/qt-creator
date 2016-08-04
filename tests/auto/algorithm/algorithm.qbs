import qbs

QtcAutotest {
    name: "Algorithm autotest"

    cpp.includePaths: [project.ide_source_tree + "/src/libs"]
    files: [
        "tst_algorithm.cpp",
    ]
}
