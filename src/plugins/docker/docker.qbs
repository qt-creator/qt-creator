QtcPlugin {
    name: "Docker"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "CmdBridgeClient" }
    Depends { name: "Core" }
    Depends { name: "Debugger"; condition: qtc.withPluginTests }
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

    Group {
        name: "Tests"
        condition: qtc.withPluginTests
        files: [
            "dockerdebuggertest.h",
            "dockerdebuggertest.cpp",
            "dockermounttest.h",
            "dockermounttest.cpp",
        ]
    }

    Group {
        name: "images"
        prefix: "images/"
        files: [
            "dockerdevice.png",
            "dockerdevice@2x.png",
            "dockerdevicesmall.png",
            "dockerdevicesmall@2x.png",
        ]
        fileTags: "qt.core.resource_data"
        Qt.core.resourcePrefix: "/docker"
    }
}
