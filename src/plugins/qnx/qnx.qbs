import qbs 1.0

QtcPlugin {
    name: "Qnx"

    Depends { name: "Qt"; submodules: ["widgets", "xml", "network"] }
    Depends { name: "QtcSsh" }
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
        "qnxdeployqtlibrariesdialog.ui",
        "qnxtoolchain.cpp",
        "qnxtoolchain.h",
        "qnx.qrc",
        "qnxbaseqtconfigwidget.cpp",
        "qnxbaseqtconfigwidget.h",
        "qnxconstants.h",
        "qnxconfiguration.cpp",
        "qnxconfiguration.h",
        "qnxanalyzesupport.cpp",
        "qnxanalyzesupport.h",
        "qnxdebugsupport.cpp",
        "qnxdebugsupport.h",
        "qnxdevice.cpp",
        "qnxdevice.h",
        "qnxdevicewizard.cpp",
        "qnxdevicewizard.h",
        "qnxdeviceprocesslist.cpp",
        "qnxdeviceprocesslist.h",
        "qnxdeviceprocesssignaloperation.cpp",
        "qnxdeviceprocesssignaloperation.h",
        "qnxdeviceprocess.cpp",
        "qnxdeviceprocess.h",
        "qnxdevicetester.cpp",
        "qnxdevicetester.h",
        "qnxsettingswidget.ui",
        "qnxconfigurationmanager.cpp",
        "qnxconfigurationmanager.h",
        "qnxsettingspage.cpp",
        "qnxsettingspage.h",
        "qnxversionnumber.cpp",
        "qnxversionnumber.h",
        "qnxplugin.cpp",
        "qnxplugin.h",
        "qnxqtversion.cpp",
        "qnxqtversion.h",
        "qnxrunconfiguration.cpp",
        "qnxrunconfiguration.h",
        "qnxutils.cpp",
        "qnxutils.h",
        "qnx_export.h",
        "slog2inforunner.cpp",
        "slog2inforunner.h",
    ]
}
