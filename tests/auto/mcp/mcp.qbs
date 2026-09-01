import qbs

QtcAutotest {
    name: "MiniHttpServer autotest"
    Depends { name: "Qt.network" }
    cpp.includePaths: base.concat(project.ide_source_tree + "/src/libs")
    files: [
        "tst_minihttpserver.cpp",
        project.ide_source_tree + "/src/libs/mcp/server/minihttpserver.h",
    ]
}
