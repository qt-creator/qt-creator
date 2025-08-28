Project {
    name: "Qt Creator"
    minimumQbsVersion: "2.0.0"
    property path ide_source_tree: path
    property pathList additionalPlugins: []
    property pathList additionalLibs: []
    property pathList additionalTools: []
    property pathList additionalAutotests: []
    property string sharedSourcesDir: path + "/src/shared"
    qbsSearchPaths: "qbs"

    references: [
        "doc/doc.qbs",
        "src/src.qbs",
        "share/share.qbs",
        "share/qtcreator/translations/translations.qbs",
        "tests/tests.qbs"
    ]

    Product {
        name: "CMake helpers"
        files: "cmake/*"
    }
}
