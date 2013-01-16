import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "UpdateInfo"

    Depends { name: "cpp" }
    Depends { name: "Core" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "Qt"; submodules: ["widgets", "xml", "network"] }

    property bool enable: false
    pluginspecreplacements: ({"UPDATEINFO_EXPERIMENTAL_STR": (enable ? "false": "true")})

    cpp.includePaths: [
        "..",
        buildDirectory,
    ]

    files: [
        "updateinfobutton.cpp",
        "updateinfobutton.h",
        "updateinfoplugin.cpp",
        "updateinfoplugin.h",
    ]
}
