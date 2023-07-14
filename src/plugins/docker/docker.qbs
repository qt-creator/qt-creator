import qbs 1.0

QtcPlugin {
    name: "Docker"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }

    files: [
        "docker_global.h", "dockertr.h",
        "dockerapi.cpp",
        "dockerapi.h",
        "dockerconstants.h",
        "dockerdevice.cpp",
        "dockerdevice.h",
        "dockerdevicewidget.cpp",
        "dockerdevicewidget.h",
        "dockerplugin.cpp",
        "dockersettings.cpp",
        "dockersettings.h",
        "kitdetector.cpp",
        "kitdetector.h",
    ]
}

