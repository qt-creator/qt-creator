TEMPLATE = lib
TARGET = RemoteLinux
QT += network

include(../../qtcreatorplugin.pri)
include(remotelinux_dependencies.pri)

HEADERS += \
    embeddedlinuxqtversion.h \
    embeddedlinuxqtversionfactory.h \
    remotelinuxplugin.h \
    remotelinux_export.h \
    linuxdeviceconfiguration.h \
    remotelinuxrunconfiguration.h \
    publickeydeploymentdialog.h \
    genericlinuxdeviceconfigurationwizard.h \
    genericlinuxdeviceconfigurationfactory.h \
    remotelinuxrunconfigurationwidget.h \
    remotelinuxrunconfigurationfactory.h \
    remotelinuxruncontrol.h \
    remotelinuxruncontrolfactory.h \
    remotelinuxdebugsupport.h \
    genericlinuxdeviceconfigurationwizardpages.h \
    deployablefile.h \
    deployablefilesperprofile.h \
    deploymentinfo.h \
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
    linuxdevicetestdialog.h \
    remotelinuxenvironmentreader.h \
    sshkeydeployer.h \
    typespecificdeviceconfigurationlistmodel.h \
    remotelinuxutils.h \
    deploymentsettingsassistant.h \
    remotelinuxdeployconfigurationwidget.h \
    profilesupdatedialog.h \
    remotelinuxcustomcommanddeployservice.h \
    remotelinuxcustomcommanddeploymentstep.h \
    genericlinuxdeviceconfigurationwidget.h \
    remotelinuxcheckforfreediskspaceservice.h \
    remotelinuxcheckforfreediskspacestep.h

SOURCES += \
    embeddedlinuxqtversion.cpp \
    embeddedlinuxqtversionfactory.cpp \
    remotelinuxplugin.cpp \
    linuxdeviceconfiguration.cpp \
    remotelinuxrunconfiguration.cpp \
    publickeydeploymentdialog.cpp \
    genericlinuxdeviceconfigurationwizard.cpp \
    genericlinuxdeviceconfigurationfactory.cpp \
    remotelinuxrunconfigurationwidget.cpp \
    remotelinuxrunconfigurationfactory.cpp \
    remotelinuxruncontrol.cpp \
    remotelinuxruncontrolfactory.cpp \
    remotelinuxdebugsupport.cpp \
    genericlinuxdeviceconfigurationwizardpages.cpp \
    deployablefilesperprofile.cpp \
    deploymentinfo.cpp \
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
    linuxdevicetestdialog.cpp \
    remotelinuxenvironmentreader.cpp \
    sshkeydeployer.cpp \
    typespecificdeviceconfigurationlistmodel.cpp \
    remotelinuxutils.cpp \
    deploymentsettingsassistant.cpp \
    remotelinuxdeployconfigurationwidget.cpp \
    profilesupdatedialog.cpp \
    remotelinuxcustomcommanddeployservice.cpp \
    remotelinuxcustomcommanddeploymentstep.cpp \
    genericlinuxdeviceconfigurationwidget.cpp \
    remotelinuxcheckforfreediskspaceservice.cpp \
    remotelinuxcheckforfreediskspacestep.cpp

FORMS += \
    genericlinuxdeviceconfigurationwizardsetuppage.ui \
    linuxdevicetestdialog.ui \
    remotelinuxdeployconfigurationwidget.ui \
    profilesupdatedialog.ui \
    genericlinuxdeviceconfigurationwidget.ui \
    remotelinuxcheckforfreediskspacestepwidget.ui

RESOURCES += remotelinux.qrc

DEFINES += QT_NO_CAST_TO_ASCII
DEFINES += REMOTELINUX_LIBRARY
