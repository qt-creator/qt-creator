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
    Depends { name: "QmakeProjectManager" }
    Depends { name: "RemoteLinux" }

    files: [
        "qnxdeployqtlibrariesdialog.cpp",
        "qnxdeployqtlibrariesdialog.h",
        "qnxdeployqtlibrariesdialog.ui",
        "pathchooserdelegate.cpp",
        "pathchooserdelegate.h",
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
        "qnxdeployconfiguration.cpp",
        "qnxdeployconfiguration.h",
        "qnxdeployconfigurationfactory.cpp",
        "qnxdeployconfigurationfactory.h",
        "qnxdeploystepfactory.cpp",
        "qnxdeploystepfactory.h",
        "qnxdevice.cpp",
        "qnxdevice.h",
        "qnxdevicefactory.cpp",
        "qnxdevicefactory.h",
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
        "qnxsettingswidget.cpp",
        "qnxsettingswidget.h",
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
        "qnxqtversionfactory.cpp",
        "qnxqtversionfactory.h",
        "qnxrunconfiguration.cpp",
        "qnxrunconfiguration.h",
        "qnxrunconfigurationfactory.cpp",
        "qnxrunconfigurationfactory.h",
        "qnxutils.cpp",
        "qnxutils.h",
        "qnx_export.h",
        "slog2inforunner.cpp",
        "slog2inforunner.h",
    ]
}
