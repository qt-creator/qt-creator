import qbs.base 1.0

import QtcPlugin

QtcPlugin {
    name: "UpdateInfo"

    Depends { name: "Core" }
    Depends { name: "Qt"; submodules: ["widgets", "xml", "network"] }

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
