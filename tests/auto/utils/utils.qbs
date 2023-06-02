import qbs

Project {
    name: "Utils autotests"
    references: [
        "ansiescapecodehandler/ansiescapecodehandler.qbs",
        "async/async.qbs",
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
        "process/process.qbs",
        "settings/settings.qbs",
        "stringutils/stringutils.qbs",
        "templateengine/templateengine.qbs",
        "text/text.qbs",
        "treemodel/treemodel.qbs",
        "unixdevicefileaccess/unixdevicefileaccess.qbs",
    ]
}
