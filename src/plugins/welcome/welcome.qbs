import qbs 1.0

QtcPlugin {
    name: "Welcome"

    Depends { name: "Qt"; submodules: ["widgets", "network" ] }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "app_version_header" }

    files: [
        "welcome.qrc",
        "welcomeplugin.cpp",
    ]
}
