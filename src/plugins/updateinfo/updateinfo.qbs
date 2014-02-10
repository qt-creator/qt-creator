import qbs.base 1.0

import QtcPlugin

QtcPlugin {
    name: "UpdateInfo"

    Depends { name: "Qt"; submodules: ["widgets", "xml", "network"] }
    Depends { name: "Utils" }

    Depends { name: "Core" }

    property bool enable: false
    pluginspecreplacements: ({"UPDATEINFO_EXPERIMENTAL_STR": (enable ? "false": "true")})

    files: [
        "updateinfobutton.cpp",
        "updateinfobutton.h",
        "updateinfoplugin.cpp",
        "updateinfoplugin.h",
        "settingspage.cpp",
        "settingspage.h",
        "settingspage.ui",
    ]
}
