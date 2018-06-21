import qbs

Project {
    name: "Utils autotests"
    references: [
        "fileutils/fileutils.qbs",
        "ansiescapecodehandler/ansiescapecodehandler.qbs",
        "fuzzymatcher/fuzzymatcher.qbs",
        "settings/settings.qbs",
        "stringutils/stringutils.qbs",
        "templateengine/templateengine.qbs",
        "treemodel/treemodel.qbs",
    ]
}
