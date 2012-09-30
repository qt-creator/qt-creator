import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "HelloWorld"

    Depends { name: "Core" }
    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ["widgets", "xml", "network", "script"] }

    cpp.includePaths: [
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "helloworldplugin.cpp",
        "helloworldplugin.h",
        "helloworldwindow.cpp",
        "helloworldwindow.h",
    ]
}

