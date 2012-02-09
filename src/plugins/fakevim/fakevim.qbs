import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "FakeVim"

    Depends { name: "aggregation" }     // ### should be injected by product dependency "Core"
    Depends { name: "extensionsystem" } // ### should be injected by product dependency "Core"
    Depends { name: "utils" }           // ### should be injected by product dependency "Core"
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "find" }
    Depends { name: "cpp" }
    Depends { name: "qt"; submodules: ['gui'] }

    cpp.includePaths: [
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "fakevimactions.cpp",
        "fakevimhandler.cpp",
        "fakevimplugin.cpp",
        "fakevimactions.h",
        "fakevimhandler.h",
        "fakevimplugin.h",
        "fakevimoptions.ui"
    ]
}

