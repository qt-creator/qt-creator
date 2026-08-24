import qbs 1.0

QtcPlugin {
    name: "HarmonyOS"

    Depends { name: "Qt.core" }
    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "CmdBridgeClient" }

    Depends { name: "CMakeProjectManager" }
    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "Remote" }

    files: [
        "harmonyosbuilddevice.cpp",
        "harmonyosbuilddevice.h",
        "harmonyosdebugsupport.cpp",
        "harmonyosdebugsupport.h",
        "harmonyosconfigurations.cpp",
        "harmonyosconfigurations.h",
        "harmonyosconstants.h",
        "harmonyosdeploystep.cpp",
        "harmonyosdeploystep.h",
        "harmonyosdevice.cpp",
        "harmonyosdevice.h",
        "harmonyosplugin.cpp",
        "harmonyosqtversion.cpp",
        "harmonyosqtversion.h",
        "harmonyosrunconfiguration.cpp",
        "harmonyosrunconfiguration.h",
        "harmonyossdk.cpp",
        "harmonyossdk.h",
        "harmonyossettings.cpp",
        "harmonyossettings.h",
        "harmonyostoolchain.cpp",
        "harmonyostoolchain.h",
        "harmonyostr.h",
    ]

    QtcTestFiles {
        files: [
            "harmonyosdevice_test.cpp",
            "harmonyosdevice_test.h",
        ]
    }
}
