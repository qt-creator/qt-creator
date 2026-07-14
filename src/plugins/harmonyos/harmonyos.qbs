import qbs 1.0

QtcPlugin {
    name: "HarmonyOS"

    Depends { name: "Qt.core" }
    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }

    files: [
        "harmonyosconfigurations.cpp",
        "harmonyosconfigurations.h",
        "harmonyosconstants.h",
        "harmonyosdevice.cpp",
        "harmonyosdevice.h",
        "harmonyosplugin.cpp",
        "harmonyosqtversion.cpp",
        "harmonyosqtversion.h",
        "harmonyossdk.cpp",
        "harmonyossdk.h",
        "harmonyossettings.cpp",
        "harmonyossettings.h",
        "harmonyostoolchain.cpp",
        "harmonyostoolchain.h",
        "harmonyostr.h",
    ]
}
