Project {
    name: "Qt Creator"
    minimumQbsVersion: "2.0.0"
    property bool withAutotests: qbs.buildVariant === "debug" // TODO: compat, remove
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
        name: "cmake project files"
        files: {
            var patterns = ["**/CMakeLists.txt", "**/*.cmake", "**/*.cmake.in"];
            var list = [].concat(patterns);
            var props = [additionalPlugins, additionalLibs, additionalTools, additionalAutotests];
            for (var i = 0; i < props.length; ++i) {
                for (var j = 0; j < props[i].length; ++j) {
                    for (var k = 0; k < patterns.length; ++k)
                        list.push(props[i][j] + "/" + patterns[k]);
                }
            }
            return list;
        }
    }
}
