import qbs 1.0

QtcPlugin {
    name: "Conan"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

    files: [
        "conanplugin.h",
        "conanplugin.cpp",
        "conaninstallstep.h",
        "conaninstallstep.cpp"
    ]
}

