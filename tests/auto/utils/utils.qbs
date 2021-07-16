import qbs

Project {
    name: "Utils autotests"
    references: [
        "fileutils/fileutils.qbs",
        "ansiescapecodehandler/ansiescapecodehandler.qbs",
        "fuzzymatcher/fuzzymatcher.qbs",
        "indexedcontainerproxyconstiterator/indexedcontainerproxyconstiterator.qbs",
        "persistentsettings/persistentsettings.qbs",
        "qtcprocess/qtcprocess.qbs",
        "settings/settings.qbs",
        "stringutils/stringutils.qbs",
        "templateengine/templateengine.qbs",
        "treemodel/treemodel.qbs",
    ]
}
