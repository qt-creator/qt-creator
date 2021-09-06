import qbs
import qbs.FileInfo

QtcTool {
    name: "qtcreator_processlauncher"

    Depends { name: "Qt.network" }

    cpp.includePaths: base.concat(pathToUtils)

    files: [
        "launcherlogging.cpp",
        "launcherlogging.h",
        "launchersockethandler.cpp",
        "launchersockethandler.h",
        "processlauncher-main.cpp",
    ]

    property string pathToUtils: sourceDirectory + "/../../libs/utils"
    Group {
        name: "protocol sources"
        prefix: pathToUtils + '/'
        files: [
            "launcherpackets.cpp",
            "launcherpackets.h",
            "processreaper.cpp",
            "processreaper.h",
            "processutils.cpp",
            "processutils.h",
            "qtcassert.cpp",
            "qtcassert.h",
        ]
    }
}
