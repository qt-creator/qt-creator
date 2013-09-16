import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Welcome"
    condition: project.buildWelcomePlugin

    Depends { name: "Qt"; submodules: ["widgets", "network", "quick"] }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

    files: [
        "welcomeplugin.cpp",
        "welcomeplugin.h",
    ]
}
