import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Welcome"

    Depends { name: "Qt"; submodules: ["widgets", "network", "quick1"] }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

    Depends { name: "cpp" }
    cpp.includePaths: base.concat("../../shared/scriptwrapper")

    files: [
        "welcome_global.h",
        "welcomeplugin.cpp",
        "welcomeplugin.h",
    ]
}
