add_qtc_plugin(RemoteLinux
  DEPENDS QmlDebug
  PLUGIN_DEPENDS Core Debugger ProjectExplorer
  SOURCES
    abstractremotelinuxdeployservice.cpp abstractremotelinuxdeployservice.h
    abstractremotelinuxdeploystep.cpp abstractremotelinuxdeploystep.h
    customcommanddeploystep.cpp customcommanddeploystep.h
    deploymenttimeinfo.cpp deploymenttimeinfo.h
    genericdirectuploadservice.cpp genericdirectuploadservice.h
    genericdirectuploadstep.cpp genericdirectuploadstep.h
    genericlinuxdeviceconfigurationwidget.cpp genericlinuxdeviceconfigurationwidget.h genericlinuxdeviceconfigurationwidget.ui
    genericlinuxdeviceconfigurationwizard.cpp genericlinuxdeviceconfigurationwizard.h
    genericlinuxdeviceconfigurationwizardpages.cpp genericlinuxdeviceconfigurationwizardpages.h
    genericlinuxdeviceconfigurationwizardsetuppage.ui
    killappstep.cpp killappstep.h
    linuxdevice.cpp linuxdevice.h
    linuxdevicetester.cpp linuxdevicetester.h
    linuxprocessinterface.h
    makeinstallstep.cpp makeinstallstep.h
    publickeydeploymentdialog.cpp publickeydeploymentdialog.h
    remotelinux.qrc
    remotelinux_constants.h
    remotelinux_export.h
    remotelinuxcustomrunconfiguration.cpp remotelinuxcustomrunconfiguration.h
    remotelinuxdebugsupport.cpp remotelinuxdebugsupport.h
    remotelinuxdeployconfiguration.cpp remotelinuxdeployconfiguration.h
    remotelinuxenvironmentaspect.cpp remotelinuxenvironmentaspect.h
    remotelinuxenvironmentaspectwidget.cpp remotelinuxenvironmentaspectwidget.h
    remotelinuxenvironmentreader.cpp remotelinuxenvironmentreader.h
    remotelinuxplugin.cpp remotelinuxplugin.h
    remotelinuxqmltoolingsupport.cpp remotelinuxqmltoolingsupport.h
    remotelinuxrunconfiguration.cpp remotelinuxrunconfiguration.h
    remotelinuxsignaloperation.cpp remotelinuxsignaloperation.h
    rsyncdeploystep.cpp rsyncdeploystep.h
    sshkeycreationdialog.cpp sshkeycreationdialog.h sshkeycreationdialog.ui
    sshprocessinterface.h
    tarpackagecreationstep.cpp tarpackagecreationstep.h
    tarpackagedeploystep.cpp tarpackagedeploystep.h
    x11forwardingaspect.cpp x11forwardingaspect.h
)

extend_qtc_plugin(RemoteLinux
  CONDITION WITH_TESTS
  SOURCES
    filesystemaccess_test.cpp
    filesystemaccess_test.h
)
