import qbs 1.0

QtcPlugin {
    name: "UpdateInfo"

    Depends { name: "Qt"; submodules: ["widgets", "xml", "network"] }
    Depends { name: "Utils" }

    Depends { name: "Core" }

    property bool enable: false
    pluginjson.replacements: ({"UPDATEINFO_EXPERIMENTAL_STR": (enable ? "false": "true")})

    files: [
        "settingspage.cpp",
        "settingspage.h",
        "updateinfoplugin.cpp",
        "updateinfoplugin.h",
        "updateinfotools.h",
        "updateinfotr.h",
    ]
}
