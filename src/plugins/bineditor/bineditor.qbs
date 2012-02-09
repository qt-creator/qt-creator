import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "BinEditor"

    Depends { name: "qt"; submodules: ['gui'] }
    Depends { name: "utils" }
    Depends { name: "extensionsystem" }
    Depends { name: "aggregation" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "find" }

    Depends { name: "cpp" }
    cpp.includePaths: [
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "bineditorplugin.h",
        "bineditor.h",
        "bineditorconstants.h",
        "markup.h",
        "bineditorplugin.cpp",
        "bineditor.cpp"
    ]
}

