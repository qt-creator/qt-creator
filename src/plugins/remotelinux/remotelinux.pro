QT += network

include(../../qtcreatorplugin.pri)

HEADERS += \
    embeddedlinuxqtversion.h \
    embeddedlinuxqtversionfactory.h \
    remotelinuxenvironmentaspect.h \
    remotelinuxenvironmentaspectwidget.h \
    remotelinuxplugin.h \
    remotelinux_export.h \
    linuxdevice.h \
    remotelinuxrunconfiguration.h \
    publickeydeploymentdialog.h \
    genericlinuxdeviceconfigurationwizard.h \
    genericlinuxdeviceconfigurationfactory.h \
    remotelinuxrunconfigurationwidget.h \
    remotelinuxrunconfigurationfactory.h \
    remotelinuxruncontrolfactory.h \
    remotelinuxdebugsupport.h \
    genericlinuxdeviceconfigurationwizardpages.h \
    abstractremotelinuxdeploystep.h \
    genericdirectuploadstep.h \
    uploadandinstalltarpackagestep.h \
    abstractremotelinuxdeployservice.h \
    abstractuploadandinstallpackageservice.h \
    genericdirectuploadservice.h \
    remotelinuxdeployconfiguration.h \
    remotelinuxdeployconfigurationfactory.h \
    genericremotelinuxdeploystepfactory.h \
    abstractpackagingstep.h \
    tarpackagecreationstep.h \
    remotelinuxpackageinstaller.h \
    packageuploader.h \
    linuxdevicetester.h \
    remotelinux_constants.h \
    remotelinuxenvironmentreader.h \
    sshkeydeployer.h \
    typespecificdeviceconfigurationlistmodel.h \
    remotelinuxutils.h \
    remotelinuxcustomcommanddeployservice.h \
    remotelinuxcustomcommanddeploymentstep.h \
    genericlinuxdeviceconfigurationwidget.h \
    remotelinuxcheckforfreediskspaceservice.h \
    remotelinuxcheckforfreediskspacestep.h \
    remotelinuxanalyzesupport.h \
    abstractremotelinuxrunsupport.h \
    linuxdeviceprocess.h \
    remotelinuxcustomrunconfiguration.h \
    remotelinuxsignaloperation.h \
    deploymenttimeinfo.h

SOURCES += \
    embeddedlinuxqtversion.cpp \
    embeddedlinuxqtversionfactory.cpp \
    remotelinuxenvironmentaspect.cpp \
    remotelinuxenvironmentaspectwidget.cpp \
    remotelinuxplugin.cpp \
    linuxdevice.cpp \
    remotelinuxrunconfiguration.cpp \
    publickeydeploymentdialog.cpp \
    genericlinuxdeviceconfigurationwizard.cpp \
    genericlinuxdeviceconfigurationfactory.cpp \
    remotelinuxrunconfigurationwidget.cpp \
    remotelinuxrunconfigurationfactory.cpp \
    remotelinuxruncontrolfactory.cpp \
    remotelinuxdebugsupport.cpp \
    genericlinuxdeviceconfigurationwizardpages.cpp \
    abstractremotelinuxdeploystep.cpp \
    genericdirectuploadstep.cpp \
    uploadandinstalltarpackagestep.cpp \
    abstractremotelinuxdeployservice.cpp \
    abstractuploadandinstallpackageservice.cpp \
    genericdirectuploadservice.cpp \
    remotelinuxdeployconfiguration.cpp \
    remotelinuxdeployconfigurationfactory.cpp \
    genericremotelinuxdeploystepfactory.cpp \
    abstractpackagingstep.cpp \
    tarpackagecreationstep.cpp \
    remotelinuxpackageinstaller.cpp \
    packageuploader.cpp \
    linuxdevicetester.cpp \
    remotelinuxenvironmentreader.cpp \
    sshkeydeployer.cpp \
    typespecificdeviceconfigurationlistmodel.cpp \
    remotelinuxutils.cpp \
    remotelinuxcustomcommanddeployservice.cpp \
    remotelinuxcustomcommanddeploymentstep.cpp \
    genericlinuxdeviceconfigurationwidget.cpp \
    remotelinuxcheckforfreediskspaceservice.cpp \
    remotelinuxcheckforfreediskspacestep.cpp \
    remotelinuxanalyzesupport.cpp \
    abstractremotelinuxrunsupport.cpp \
    linuxdeviceprocess.cpp \
    remotelinuxcustomrunconfiguration.cpp \
    remotelinuxsignaloperation.cpp \
    deploymenttimeinfo.cpp

FORMS += \
    genericlinuxdeviceconfigurationwizardsetuppage.ui \
    genericlinuxdeviceconfigurationwidget.ui \
    remotelinuxcheckforfreediskspacestepwidget.ui \
    remotelinuxcustomrunconfigurationwidget.ui

RESOURCES += remotelinux.qrc

DEFINES += REMOTELINUX_LIBRARY
