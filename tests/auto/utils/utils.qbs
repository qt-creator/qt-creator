import qbs

Project {
    name: "Utils autotests"
    references: [
        "fileutils/fileutils.qbs",
        "ansiescapecodehandler/ansiescapecodehandler.qbs",
        "camelhumpmatcher/camelhumpmatcher.qbs",
        "stringutils/stringutils.qbs",
        "objectpool/objectpool.qbs",
        "templateengine/templateengine.qbs",
        "treemodel/treemodel.qbs",
    ]
}
