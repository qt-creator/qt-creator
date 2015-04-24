import qbs 1.0

QtcPlugin {
    name: "Qnx"

    Depends { name: "Qt"; submodules: ["widgets", "xml", "network"] }
    Depends { name: "QtcSsh" }
    Depends { name: "QmlDebug" }
    Depends { name: "Utils" }

    Depends { name: "AnalyzerBase" }
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
        "qnxattachdebugsupport.cpp",
        "qnxattachdebugsupport.h",
        "qnxattachdebugdialog.cpp",
        "qnxattachdebugdialog.h",
        "qnxbaseqtconfigwidget.cpp",
        "qnxbaseqtconfigwidget.h",
        "qnxconstants.h",
        "qnxconfiguration.cpp",
        "qnxconfiguration.h",
        "qnxabstractrunsupport.cpp",
        "qnxabstractrunsupport.h",
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
        "qnxdeviceconfiguration.cpp",
        "qnxdeviceconfiguration.h",
        "qnxdeviceconfigurationfactory.cpp",
        "qnxdeviceconfigurationfactory.h",
        "qnxdeviceconfigurationwizard.cpp",
        "qnxdeviceconfigurationwizard.h",
        "qnxdeviceconfigurationwizardpages.cpp",
        "qnxdeviceconfigurationwizardpages.h",
        "qnxdeviceprocesslist.cpp",
        "qnxdeviceprocesslist.h",
        "qnxdeviceprocesssignaloperation.cpp",
        "qnxdeviceprocesssignaloperation.h",
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
        "qnxruncontrol.cpp",
        "qnxruncontrol.h",
        "qnxruncontrolfactory.cpp",
        "qnxruncontrolfactory.h",
        "qnxutils.cpp",
        "qnxutils.h",
        "slog2inforunner.cpp",
        "slog2inforunner.h",
    ]
}
