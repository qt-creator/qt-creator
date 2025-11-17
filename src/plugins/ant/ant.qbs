import qbs 1.0

QtcPlugin {
    name: "Ant"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Qt"; submodules: ["widgets"] }

    files: [
        "antplugin.cpp",
        "antproject.cpp",
        "antproject.h",
    ]
}

