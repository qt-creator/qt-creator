import qbs 1.0

QtcPlugin {
    name: "Dotnet"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Qt"; submodules: ["widgets"] }

    files: [
        "dotnetplugin.cpp",
        "dotnetproject.cpp",
        "dotnetproject.h",
    ]
}

