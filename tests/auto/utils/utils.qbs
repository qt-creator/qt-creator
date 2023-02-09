import qbs

Project {
    name: "Utils autotests"
    references: [
        "ansiescapecodehandler/ansiescapecodehandler.qbs",
        "asynctask/asynctask.qbs",
        "commandline/commandline.qbs",
        "deviceshell/deviceshell.qbs",
        "expected/expected.qbs",
        "filepath/filepath.qbs",
        "fileutils/fileutils.qbs",
        "fsengine/fsengine.qbs",
        "fuzzymatcher/fuzzymatcher.qbs",
        "indexedcontainerproxyconstiterator/indexedcontainerproxyconstiterator.qbs",
        "mathutils/mathutils.qbs",
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
