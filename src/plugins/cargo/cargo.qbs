import qbs 1.0

QtcPlugin {
    name: "Cargo"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Qt"; submodules: ["widgets"] }

    files: [
        "cargoplugin.cpp",
        "cargoproject.cpp",
        "cargoproject.h",
    ]
}

