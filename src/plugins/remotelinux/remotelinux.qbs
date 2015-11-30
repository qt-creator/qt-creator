import qbs 1.0

QtcPlugin {
    name: "RemoteLinux"

    Depends { name: "Qt.widgets" }
    Depends { name: "QtcSsh" }
    Depends { name: "QmlDebug" }
    Depends { name: "Utils" }

    Depends { name: "AnalyzerBase" }
    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }

    files: [
        "abstractpackagingstep.cpp",
        "abstractpackagingstep.h",
        "abstractremotelinuxdeployservice.cpp",
        "abstractremotelinuxdeployservice.h",
        "abstractremotelinuxdeploystep.cpp",
        "abstractremotelinuxdeploystep.h",
        "abstractremotelinuxrunsupport.cpp",
        "abstractremotelinuxrunsupport.h",
        "abstractremotelinuxrunconfiguration.h",
        "abstractremotelinuxrunconfiguration.cpp",
        "abstractuploadandinstallpackageservice.cpp",
        "abstractuploadandinstallpackageservice.h",
        "embeddedlinuxqtversion.cpp",
        "embeddedlinuxqtversion.h",
        "embeddedlinuxqtversionfactory.cpp",
        "embeddedlinuxqtversionfactory.h",
        "genericdirectuploadservice.cpp",
        "genericdirectuploadservice.h",
        "genericdirectuploadstep.cpp",
        "genericdirectuploadstep.h",
        "genericlinuxdeviceconfigurationfactory.cpp",
        "genericlinuxdeviceconfigurationfactory.h",
        "genericlinuxdeviceconfigurationwidget.cpp",
        "genericlinuxdeviceconfigurationwidget.h",
        "genericlinuxdeviceconfigurationwidget.ui",
        "genericlinuxdeviceconfigurationwizard.cpp",
        "genericlinuxdeviceconfigurationwizard.h",
        "genericlinuxdeviceconfigurationwizardpages.cpp",
        "genericlinuxdeviceconfigurationwizardpages.h",
        "genericlinuxdeviceconfigurationwizardsetuppage.ui",
        "genericremotelinuxdeploystepfactory.cpp",
        "genericremotelinuxdeploystepfactory.h",
        "linuxdevice.cpp",
        "linuxdevice.h",
        "linuxdeviceprocess.cpp",
        "linuxdeviceprocess.h",
        "linuxdevicetester.cpp",
        "linuxdevicetester.h",
        "packageuploader.cpp",
        "packageuploader.h",
        "publickeydeploymentdialog.cpp",
        "publickeydeploymentdialog.h",
        "remotelinux.qrc",
        "remotelinux_constants.h",
        "remotelinux_export.h",
        "remotelinuxanalyzesupport.cpp",
        "remotelinuxanalyzesupport.h",
        "remotelinuxcheckforfreediskspaceservice.cpp",
        "remotelinuxcheckforfreediskspaceservice.h",
        "remotelinuxcheckforfreediskspacestep.cpp",
        "remotelinuxcheckforfreediskspacestep.h",
        "remotelinuxcheckforfreediskspacestepwidget.ui",
        "remotelinuxcustomcommanddeploymentstep.cpp",
        "remotelinuxcustomcommanddeploymentstep.h",
        "remotelinuxcustomcommanddeployservice.cpp",
        "remotelinuxcustomcommanddeployservice.h",
        "remotelinuxcustomrunconfiguration.cpp",
        "remotelinuxcustomrunconfiguration.h",
        "remotelinuxcustomrunconfigurationwidget.ui",
        "remotelinuxdebugsupport.cpp",
        "remotelinuxdebugsupport.h",
        "remotelinuxdeployconfiguration.cpp",
        "remotelinuxdeployconfiguration.h",
        "remotelinuxdeployconfigurationfactory.cpp",
        "remotelinuxdeployconfigurationfactory.h",
        "remotelinuxenvironmentaspect.cpp",
        "remotelinuxenvironmentaspect.h",
        "remotelinuxenvironmentaspectwidget.cpp",
        "remotelinuxenvironmentaspectwidget.h",
        "remotelinuxenvironmentreader.cpp",
        "remotelinuxenvironmentreader.h",
        "remotelinuxpackageinstaller.cpp",
        "remotelinuxpackageinstaller.h",
        "remotelinuxplugin.cpp",
        "remotelinuxplugin.h",
        "remotelinuxrunconfiguration.cpp",
        "remotelinuxrunconfiguration.h",
        "remotelinuxrunconfigurationfactory.cpp",
        "remotelinuxrunconfigurationfactory.h",
        "remotelinuxrunconfigurationwidget.cpp",
        "remotelinuxrunconfigurationwidget.h",
        "remotelinuxruncontrol.cpp",
        "remotelinuxruncontrol.h",
        "remotelinuxruncontrolfactory.cpp",
        "remotelinuxruncontrolfactory.h",
        "remotelinuxsignaloperation.cpp",
        "remotelinuxsignaloperation.h",
        "remotelinuxutils.cpp",
        "remotelinuxutils.h",
        "sshkeydeployer.cpp",
        "sshkeydeployer.h",
        "tarpackagecreationstep.cpp",
        "tarpackagecreationstep.h",
        "typespecificdeviceconfigurationlistmodel.cpp",
        "typespecificdeviceconfigurationlistmodel.h",
        "uploadandinstalltarpackagestep.cpp",
        "uploadandinstalltarpackagestep.h",
        "images/embeddedtarget.png"
    ]

    Export {
        Depends { name: "AnalyzerBase" }
        Depends { name: "Core" }
        Depends { name: "QtcSsh" }
    }
}

