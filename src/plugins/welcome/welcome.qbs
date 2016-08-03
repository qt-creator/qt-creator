import qbs 1.0

QtcPlugin {
    name: "Welcome"

    Depends { name: "Qt"; submodules: ["widgets", "network", "quick", "quickwidgets"] }
    Depends { name: "Utils" }

    Depends { name: "Core" }

    files: [
        "welcomeplugin.cpp",
        "welcomeplugin.h",
        "welcome.qrc",
    ]
}
