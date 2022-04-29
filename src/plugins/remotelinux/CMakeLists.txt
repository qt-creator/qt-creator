add_qtc_plugin(RemoteLinux
  DEPENDS QmlDebug QtcSsh
  PLUGIN_DEPENDS Core Debugger ProjectExplorer
  SOURCES
    abstractpackagingstep.cpp abstractpackagingstep.h
    abstractremotelinuxdeployservice.cpp abstractremotelinuxdeployservice.h
    abstractremotelinuxdeploystep.cpp abstractremotelinuxdeploystep.h
    abstractuploadandinstallpackageservice.cpp abstractuploadandinstallpackageservice.h
    deploymenttimeinfo.cpp deploymenttimeinfo.h
    genericdirectuploadservice.cpp genericdirectuploadservice.h
    genericdirectuploadstep.cpp genericdirectuploadstep.h
    genericlinuxdeviceconfigurationwidget.cpp genericlinuxdeviceconfigurationwidget.h genericlinuxdeviceconfigurationwidget.ui
    genericlinuxdeviceconfigurationwizard.cpp genericlinuxdeviceconfigurationwizard.h
    genericlinuxdeviceconfigurationwizardpages.cpp genericlinuxdeviceconfigurationwizardpages.h
    genericlinuxdeviceconfigurationwizardsetuppage.ui
    linuxdevice.cpp linuxdevice.h
    linuxdeviceprocess.cpp linuxdeviceprocess.h
    linuxdevicetester.cpp linuxdevicetester.h
    linuxprocessinterface.h
    makeinstallstep.cpp makeinstallstep.h
    packageuploader.cpp packageuploader.h
    publickeydeploymentdialog.cpp publickeydeploymentdialog.h
    remotelinux.qrc
    remotelinux_constants.h
    remotelinux_export.h
    remotelinuxcheckforfreediskspaceservice.cpp remotelinuxcheckforfreediskspaceservice.h
    remotelinuxcheckforfreediskspacestep.cpp remotelinuxcheckforfreediskspacestep.h
    remotelinuxcustomcommanddeploymentstep.cpp remotelinuxcustomcommanddeploymentstep.h
    remotelinuxcustomcommanddeployservice.cpp remotelinuxcustomcommanddeployservice.h
    remotelinuxcustomrunconfiguration.cpp remotelinuxcustomrunconfiguration.h
    remotelinuxdebugsupport.cpp remotelinuxdebugsupport.h
    remotelinuxdeployconfiguration.cpp remotelinuxdeployconfiguration.h
    remotelinuxenvironmentaspect.cpp remotelinuxenvironmentaspect.h
    remotelinuxenvironmentaspectwidget.cpp remotelinuxenvironmentaspectwidget.h
    remotelinuxenvironmentreader.cpp remotelinuxenvironmentreader.h
    remotelinuxkillappservice.cpp remotelinuxkillappservice.h
    remotelinuxkillappstep.cpp remotelinuxkillappstep.h
    remotelinuxpackageinstaller.cpp remotelinuxpackageinstaller.h
    remotelinuxplugin.cpp remotelinuxplugin.h
    remotelinuxqmltoolingsupport.cpp remotelinuxqmltoolingsupport.h
    remotelinuxrunconfiguration.cpp remotelinuxrunconfiguration.h
    remotelinuxsignaloperation.cpp remotelinuxsignaloperation.h
    remotelinuxx11forwardingaspect.cpp remotelinuxx11forwardingaspect.h
    rsyncdeploystep.cpp rsyncdeploystep.h
    sshkeydeployer.cpp sshkeydeployer.h
    sshprocessinterface.h
    tarpackagecreationstep.cpp tarpackagecreationstep.h
    uploadandinstalltarpackagestep.cpp uploadandinstalltarpackagestep.h
)

extend_qtc_plugin(RemoteLinux
  CONDITION WITH_TESTS
  SOURCES
    filesystemaccess_test.cpp
    filesystemaccess_test.h
)
