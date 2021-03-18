import qbs 1.0

QtcPlugin {
    name: "Docker"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

    files: [
        "docker_global.h",
        "dockerconstants.h",
        "dockerplugin.h",
        "dockerplugin.cpp",
        "dockersettings.h",
        "dockersettings.cpp"
    ]
}

