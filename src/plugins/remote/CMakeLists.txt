add_qtc_plugin(Remote
  DEPENDS QmlDebug CmdBridgeClient
  PLUGIN_DEPENDS Core Debugger ProjectExplorer
  SOURCES
    abstractremotelinuxdeploystep.cpp abstractremotelinuxdeploystep.h
    connectdevicestep.cpp connectdevicestep.h
    customcommanddeploystep.cpp customcommanddeploystep.h
    deploymenttimeinfo.cpp deploymenttimeinfo.h
    genericdeploystep.cpp genericdeploystep.h
    genericdirectuploadstep.cpp genericdirectuploadstep.h
    killappstep.cpp killappstep.h
    linuxdevice.cpp linuxdevice.h
    linuxdevicetester.cpp linuxdevicetester.h
    linuxprocessinterface.h
    macdevice.cpp macdevice.h
    makeinstallstep.cpp makeinstallstep.h
    powershellutils.h
    publickeydeploymentdialog.cpp publickeydeploymentdialog.h
    remotelinux_constants.h
    remotelinux_export.h
    remotelinuxcustomrunconfiguration.cpp remotelinuxcustomrunconfiguration.h
    remotelinuxdebugsupport.cpp remotelinuxdebugsupport.h
    remotelinuxdeploysupport.cpp remotelinuxdeploysupport.h
    remotelinuxenvironmentaspect.cpp remotelinuxenvironmentaspect.h
    remotelinuxfiletransfer.cpp remotelinuxfiletransfer.h
    remotelinuxplugin.cpp
    remotelinuxrunconfiguration.cpp remotelinuxrunconfiguration.h
    remotelinuxtr.h
    sshconnectionsharing.cpp sshconnectionsharing.h
    sshdevicewizard.cpp sshdevicewizard.h
    sshkeycreationdialog.cpp sshkeycreationdialog.h
    tarpackagecreationstep.cpp tarpackagecreationstep.h
    tarpackagedeploystep.cpp tarpackagedeploystep.h
    windowsdevice.cpp windowsdevice.h
    windowsdevicetester.cpp windowsdevicetester.h
)

extend_qtc_plugin(Remote
  CONDITION WITH_TESTS
  SOURCES
    filesystemaccess_test.cpp
    filesystemaccess_test.h
    remoterun_test.cpp
    remoterun_test.h
    windowsdevicedetection_test.cpp
    windowsdevicedetection_test.h
)

qtc_add_resources(Remote "images"
  PREFIX "/remotelinux"
  FILES
    images/embeddedtarget.png
    images/macosdevice.png
    images/macosdevice@2x.png
    images/macosdevicesmall.png
    images/macosdevicesmall@2x.png
    images/windowsdevice.png
    images/windowsdevice@2x.png
    images/windowsdevicesmall.png
    images/windowsdevicesmall@2x.png
    images/linuxdevice.png
    images/linuxdevice@2x.png
    images/linuxdevicesmall.png
    images/linuxdevicesmall@2x.png
)
