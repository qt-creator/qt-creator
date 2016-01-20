import qbs

QtcAutotest {
    name: "Run extensions autotest"
    cpp.includePaths: base.concat(project.ide_source_tree + "/src/libs")

    files: [
        "tst_runextensions.cpp",
    ]
}
