import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "RemoteLinux"

    Depends { name: "cpp" }
    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "QtcSsh" }
    cpp.defines: base.concat("REMOTELINUX_LIBRARY")

    files: [
        "abstractpackagingstep.cpp",
        "abstractpackagingstep.h",
        "abstractremotelinuxdeployservice.cpp",
        "abstractremotelinuxdeployservice.h",
        "abstractremotelinuxdeploystep.cpp",
        "abstractremotelinuxdeploystep.h",
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
        "linuxdevicetestdialog.cpp",
        "linuxdevicetestdialog.h",
        "linuxdevicetestdialog.ui",
        "linuxdevicetester.cpp",
        "linuxdevicetester.h",
        "packageuploader.cpp",
        "packageuploader.h",
        "publickeydeploymentdialog.cpp",
        "publickeydeploymentdialog.h",
        "remotelinux.qrc",
        "remotelinux_constants.h",
        "remotelinux_export.h",
        "remotelinuxcheckforfreediskspaceservice.cpp",
        "remotelinuxcheckforfreediskspaceservice.h",
        "remotelinuxcheckforfreediskspacestep.cpp",
        "remotelinuxcheckforfreediskspacestep.h",
        "remotelinuxcheckforfreediskspacestepwidget.ui",
        "remotelinuxcustomcommanddeploymentstep.cpp",
        "remotelinuxcustomcommanddeploymentstep.h",
        "remotelinuxcustomcommanddeployservice.cpp",
        "remotelinuxcustomcommanddeployservice.h",
        "remotelinuxdebugsupport.cpp",
        "remotelinuxdebugsupport.h",
        "remotelinuxdeployconfiguration.cpp",
        "remotelinuxdeployconfiguration.h",
        "remotelinuxdeployconfigurationfactory.cpp",
        "remotelinuxdeployconfigurationfactory.h",
        "remotelinuxdeployconfigurationwidget.cpp",
        "remotelinuxdeployconfigurationwidget.h",
        "remotelinuxdeployconfigurationwidget.ui",
        "remotelinuxdeploymentdatamodel.cpp",
        "remotelinuxdeploymentdatamodel.h",
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
        "images/embeddedtarget.png",
    ]

    ProductModule {
        Depends { name: "Core" }
        Depends { name: "QtcSsh" }
    }
}

