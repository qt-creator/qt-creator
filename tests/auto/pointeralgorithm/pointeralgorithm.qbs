import qbs

QtcAutotest {
    name: "PointerAlgorithm autotest"

    cpp.includePaths: [project.ide_source_tree + "/src/libs"]
    files: [
        "tst_pointeralgorithm.cpp",
    ]
}
