QtcPlugin {
    name: "Docker"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "CmdBridgeClient" }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }

    files: [
        "docker_global.h", "dockertr.h",
        "dockerapi.cpp",
        "dockerapi.h",
        "dockerconstants.h",
        "dockercontainerthread.cpp",
        "dockercontainerthread.h",
        "dockerdevice.cpp",
        "dockerdevice.h",
        "dockerdeviceenvironmentaspect.cpp",
        "dockerdeviceenvironmentaspect.h",
        "dockerdevicewidget.cpp",
        "dockerdevicewidget.h",
        "dockerplugin.cpp",
        "dockersettings.cpp",
        "dockersettings.h",
    ]
}

