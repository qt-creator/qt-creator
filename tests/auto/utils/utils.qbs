import qbs

Project {
    name: "Utils autotests"
    references: [
        "ansiescapecodehandler/ansiescapecodehandler.qbs",
        "asynctask/asynctask.qbs",
        "deviceshell/deviceshell.qbs",
        "fileutils/fileutils.qbs",
        "fsengine/fsengine.qbs",
        "fuzzymatcher/fuzzymatcher.qbs",
        "indexedcontainerproxyconstiterator/indexedcontainerproxyconstiterator.qbs",
        "multicursor/multicursor.qbs",
        "persistentsettings/persistentsettings.qbs",
        "qtcprocess/qtcprocess.qbs",
        "settings/settings.qbs",
        "stringutils/stringutils.qbs",
        "tasktree/tasktree.qbs",
        "templateengine/templateengine.qbs",
        "treemodel/treemodel.qbs",
    ]
}
