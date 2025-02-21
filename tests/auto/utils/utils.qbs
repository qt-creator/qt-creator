import qbs

Project {
    name: "Utils autotests"
    references: [
        "ansiescapecodehandler/ansiescapecodehandler.qbs",
        "async/async.qbs",
        "commandline/commandline.qbs",
        "deviceshell/deviceshell.qbs",
        "expander/expander.qbs",
        "expected/expected.qbs",
        "filepath/filepath.qbs",
        "fileinprojectfinder/fileinprojectfinder.qbs",
        "fileutils/fileutils.qbs",
        "fsengine/fsengine.qbs",
        "fuzzymatcher/fuzzymatcher.qbs",
        "id/id.qbs",
        "indexedcontainerproxyconstiterator/indexedcontainerproxyconstiterator.qbs",
        "mathutils/mathutils.qbs",
        "multicursor/multicursor.qbs",
        "persistentsettings/persistentsettings.qbs",
        "process/process.qbs",
        "settings/settings.qbs",
        "stringutils/stringutils.qbs",
        "synchronizedvalue/synchronizedvalue.qbs",
        "templateengine/templateengine.qbs",
        "text/text.qbs",
        "treemodel/treemodel.qbs",
        "unixdevicefileaccess/unixdevicefileaccess.qbs",
    ]
}
