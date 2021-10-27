import qbs 1.0

QtcPlugin {
    name: "Docker"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }

    files: [
        "docker_global.h",
        "dockerbuildstep.h",
        "dockerbuildstep.cpp",
        "dockerconstants.h",
        "dockerdevice.h",
        "dockerdevice.cpp",
        "dockerplugin.h",
        "dockerplugin.cpp",
        "dockersettings.h",
        "dockersettings.cpp"
    ]
}

