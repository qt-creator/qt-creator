import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Welcome"

    Depends { name: "Qt"; submodules: ["widgets", "network", "declarative"] }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

    cpp.includePaths: base.concat("../../shared/scriptwrapper")

    files: [
        "welcome_global.h",
        "welcomeplugin.cpp",
        "welcomeplugin.h",
    ]
}
