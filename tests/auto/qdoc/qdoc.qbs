import qbs

QtcAutotest {
    name: "QDoc autotest"
    Depends { name: "Utils" }

    property string qdocDir: project.ide_source_tree + "/src/plugins/qdoc/"

    cpp.includePaths: base.concat([qdocDir])
    cpp.defines: base.concat(['QDOC_TEST_CORPUS_DIR="' + project.ide_source_tree + '/doc"'])

    Group {
        name: "Sources from QDoc plugin"
        prefix: qdocDir
        files: [
            "qdocconfig.cpp", "qdocconfig.h",
            "qdocparser.cpp", "qdocparser.h",
            "qdocrenderer.cpp", "qdocrenderer.h",
        ]
    }
    Group {
        name: "Test sources"
        files: ["tst_qdoc.cpp"]
    }
}
