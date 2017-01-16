import qbs 1.0

QtcPlugin {
    name: "Welcome"

    Depends { name: "Qt"; submodules: ["widgets", "network" ] }
    Depends { name: "Utils" }

    Depends { name: "Core" }

    files: [
        "welcome.qrc",
        "welcomeplugin.cpp",
    ]
}
