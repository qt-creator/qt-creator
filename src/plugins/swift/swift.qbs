import qbs 1.0

QtcPlugin {
    name: "Swift"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "LanguageClient" }
    Depends { name: "Qt"; submodules: ["widgets"] }

    files: [
        "swiftplugin.cpp",
        "swiftproject.cpp",
        "swiftproject.h",
        "swifttr.h",
    ]
}

