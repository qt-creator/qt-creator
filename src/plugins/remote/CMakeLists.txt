add_qtc_plugin(Remote
  DEPENDS CmdBridgeClient
  PLUGIN_DEPENDS Core Debugger ProjectExplorer
  SOURCES
    abstractremotelinuxdeploystep.cpp abstractremotelinuxdeploystep.h
    bridgeddeviceaccess.cpp bridgeddeviceaccess.h
    connectdevicestep.cpp connectdevicestep.h
    customcommanddeploystep.cpp customcommanddeploystep.h
    deploymenttimeinfo.cpp deploymenttimeinfo.h
    flatpakapi.cpp flatpakapi.h
    flatpakdevice.cpp flatpakdevice.h
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
    shellcommand.cpp shellcommand.h
    sshconnectionsharing.cpp sshconnectionsharing.h
    sshdevicewizard.cpp sshdevicewizard.h
    sshkeycreationdialog.cpp sshkeycreationdialog.h
    tarpackagecreationstep.cpp tarpackagecreationstep.h
    tarpackagedeploystep.cpp tarpackagedeploystep.h
    windowsdevice.cpp windowsdevice.h
    windowsdevicetester.cpp windowsdevicetester.h
    wslapi.cpp wslapi.h
    wslconstants.h
    wsldevice.cpp wsldevice.h
    wsldevicewidget.cpp wsldevicewidget.h
    wsltr.h
)

extend_qtc_plugin(Remote
  CONDITION WITH_TESTS
  SOURCES
    filesystemaccess_test.cpp
    filesystemaccess_test.h
    flatpakapi_test.cpp
    flatpakapi_test.h
    remoterun_test.cpp
    remoterun_test.h
    windowsdevicedetection_test.cpp
    windowsdevicedetection_test.h
)

add_qtc_test(tst_wsl_api
  CONDITION WITH_TESTS
  DEPENDS Utils
  SOURCES
    shellcommand.cpp
    shellcommand.h
    tests/tst_wsl_api.cpp
    wslapi.cpp
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
