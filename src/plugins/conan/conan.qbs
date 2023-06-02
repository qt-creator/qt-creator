import qbs 1.0

QtcPlugin {
    name: "Conan"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

    files: [
        "conanconstants.h",
        "conaninstallstep.h",
        "conaninstallstep.cpp",
        "conanplugin.cpp",
        "conansettings.h",
        "conansettings.cpp",
        "conantr.h",
    ]
}

