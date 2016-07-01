import qbs 1.0

QtcPlugin {
    name: "AppManager"

    Depends { name: "Core" }
    Depends { name: "Qt"; submodules: ["widgets", "network"] }

    files: [
        "appmanagerplugin.h", "appmanagerplugin.cpp",
    ]
}

