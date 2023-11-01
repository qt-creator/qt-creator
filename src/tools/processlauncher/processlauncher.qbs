import qbs
import qbs.FileInfo

QtcTool {
    name: "qtcreator_processlauncher"

    Depends { name: "Qt.network" }

    cpp.defines: base.concat("UTILS_STATIC_LIBRARY")
    cpp.includePaths: base.concat(pathToLibs)

    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.dynamicLibraries: qbs.toolchain.contains("msvc") ? ["user32", "dbghelp"] : ["user32"]
    }

    files: [
        "launcherlogging.cpp",
        "launcherlogging.h",
        "launchersockethandler.cpp",
        "launchersockethandler.h",
        "processlauncher-main.cpp",
    ]

    property string pathToLibs: sourceDirectory + "/../../libs"
    property string pathToUtils: sourceDirectory + "/../../libs/utils"
    Group {
        name: "protocol sources"
        prefix: pathToUtils + '/'
        files: [
            "launcherpackets.cpp",
            "launcherpackets.h",
            "processenums.h",
            "processreaper.cpp",
            "processreaper.h",
            "processutils.cpp",
            "processutils.h",
            "qtcassert.cpp",
            "qtcassert.h",
            "singleton.cpp",
            "singleton.h",
            "threadutils.cpp",
            "threadutils.h",
        ]
    }
}
