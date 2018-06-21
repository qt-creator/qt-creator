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
    remotelinuxdebugsupport.h \
    genericlinuxdeviceconfigurationwizardpages.h \
    abstractremotelinuxdeploystep.h \
    genericdirectuploadstep.h \
    uploadandinstalltarpackagestep.h \
    abstractremotelinuxdeployservice.h \
    abstractuploadandinstallpackageservice.h \
    genericdirectuploadservice.h \
    remotelinuxdeployconfiguration.h \
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
    remotelinuxkillappservice.h \
    remotelinuxkillappstep.h \
    remotelinuxqmltoolingsupport.h \
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
    remotelinuxdebugsupport.cpp \
    genericlinuxdeviceconfigurationwizardpages.cpp \
    abstractremotelinuxdeploystep.cpp \
    genericdirectuploadstep.cpp \
    uploadandinstalltarpackagestep.cpp \
    abstractremotelinuxdeployservice.cpp \
    abstractuploadandinstallpackageservice.cpp \
    genericdirectuploadservice.cpp \
    remotelinuxdeployconfiguration.cpp \
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
    remotelinuxkillappservice.cpp \
    remotelinuxkillappstep.cpp \
    remotelinuxqmltoolingsupport.cpp \
    linuxdeviceprocess.cpp \
    remotelinuxcustomrunconfiguration.cpp \
    remotelinuxsignaloperation.cpp \
    deploymenttimeinfo.cpp

FORMS += \
    genericlinuxdeviceconfigurationwizardsetuppage.ui \
    genericlinuxdeviceconfigurationwidget.ui \
    remotelinuxcheckforfreediskspacestepwidget.ui

RESOURCES += remotelinux.qrc

DEFINES += REMOTELINUX_LIBRARY
