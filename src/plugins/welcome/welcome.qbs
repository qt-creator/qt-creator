import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Welcome"

    Depends { name: "qt"; submodules: ['gui', 'network', 'declarative'] }
    Depends { name: "utils" }
    Depends { name: "extensionsystem" }
    Depends { name: "aggregation" }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

    Depends { name: "cpp" }
    cpp.defines: project.additionalCppDefines
    cpp.includePaths: [
        "..",
        "../../libs",
        "../../../src/shared/scriptwrapper",
        "../../Core/dynamiclibrary",
        buildDirectory
    ]

    files: [
        "welcome_global.h",
        "welcomeplugin.cpp",
        "welcomeplugin.h"
    ]
}

