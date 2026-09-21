QtcPlugin {
    name: "Wsl"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "CmdBridgeClient" }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }

    files: [
        "wslapi.cpp",
        "wslapi.h",
        "wslconstants.h",
        "wsldevice.cpp",
        "wsldevice.h",
        "wsldevicewidget.cpp",
        "wsldevicewidget.h",
        "wslplugin.cpp",
        "wsltr.h",
    ]

    QtcAutotest {
        name: "WSL API test"
        Depends { name: "Utils" }
        files: [
            "tests/tst_wsl_api.cpp",
            "wslapi.cpp",
        ]
    }
}
