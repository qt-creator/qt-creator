import qbs 1.0

QtcPlugin {
    name: "Qnx"

    Depends { name: "Qt"; submodules: ["widgets", "xml", "network"] }
    Depends { name: "QmlDebug" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "RemoteLinux" }

    files: [
        "qnxdeployqtlibrariesdialog.cpp",
        "qnxdeployqtlibrariesdialog.h",
        "qnxtoolchain.cpp",
        "qnxtoolchain.h",
        "qnx.qrc",
        "qnxconstants.h",
        "qnxanalyzesupport.cpp",
        "qnxanalyzesupport.h",
        "qnxdebugsupport.cpp",
        "qnxdebugsupport.h",
        "qnxdevice.cpp",
        "qnxdevice.h",
        "qnxdevicetester.cpp",
        "qnxdevicetester.h",
        "qnxsettingspage.cpp",
        "qnxsettingspage.h",
        "qnxtr.h",
        "qnxversionnumber.cpp",
        "qnxversionnumber.h",
        "qnxplugin.cpp",
        "qnxqtversion.cpp",
        "qnxqtversion.h",
        "qnxrunconfiguration.cpp",
        "qnxrunconfiguration.h",
        "qnxutils.cpp",
        "qnxutils.h",
        "slog2inforunner.cpp",
        "slog2inforunner.h",
    ]
}
