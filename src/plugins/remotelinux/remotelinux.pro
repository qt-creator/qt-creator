TEMPLATE = lib
TARGET = RemoteLinux

include(../../qtcreatorplugin.pri)
include(remotelinux_dependencies.pri)

HEADERS += \
    embeddedlinuxtarget.h \
    embeddedlinuxtargetfactory.h \
    embeddedlinuxqtversion.h \
    embeddedlinuxqtversionfactory.h \
    remotelinuxplugin.h \
    remotelinux_export.h \
    linuxdeviceconfiguration.h \
    linuxdeviceconfigurations.h \
    remotelinuxrunconfiguration.h \
    linuxdevicefactoryselectiondialog.h \
    publickeydeploymentdialog.h \
    genericlinuxdeviceconfigurationwizard.h \
    genericlinuxdeviceconfigurationfactory.h \
    remotelinuxrunconfigurationwidget.h \
    remotelinuxrunconfigurationfactory.h \
    remotelinuxapplicationrunner.h \
    remotelinuxruncontrol.h \
    remotelinuxruncontrolfactory.h \
    remotelinuxdebugsupport.h \
    genericlinuxdeviceconfigurationwizardpages.h \
    portlist.h \
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
    remotelinuxprocesslist.h \
    remotelinuxprocessesdialog.h \
    remotelinuxdeploystepwidget.h \
    linuxdeviceconfigurationssettingswidget.h \
    remotelinuxenvironmentreader.h \
    sshkeydeployer.h \
    typespecificdeviceconfigurationlistmodel.h \
    sshkeycreationdialog.h \
    remotelinuxusedportsgatherer.h \
    remotelinuxsettingspages.h \
    remotelinuxutils.h \
    deploymentsettingsassistant.h \
    remotelinuxdeployconfigurationwidget.h \
    profilesupdatedialog.h \
    startgdbserverdialog.h \
    remotelinuxcustomcommanddeployservice.h \
    remotelinuxcustomcommanddeploymentstep.h

SOURCES += \
    embeddedlinuxtarget.cpp \
    embeddedlinuxtargetfactory.cpp \
    embeddedlinuxqtversion.cpp \
    embeddedlinuxqtversionfactory.cpp \
    remotelinuxplugin.cpp \
    linuxdeviceconfiguration.cpp \
    linuxdeviceconfigurations.cpp \
    remotelinuxrunconfiguration.cpp \
    linuxdevicefactoryselectiondialog.cpp \
    publickeydeploymentdialog.cpp \
    genericlinuxdeviceconfigurationwizard.cpp \
    genericlinuxdeviceconfigurationfactory.cpp \
    remotelinuxrunconfigurationwidget.cpp \
    remotelinuxrunconfigurationfactory.cpp \
    remotelinuxapplicationrunner.cpp \
    remotelinuxruncontrol.cpp \
    remotelinuxruncontrolfactory.cpp \
    remotelinuxdebugsupport.cpp \
    genericlinuxdeviceconfigurationwizardpages.cpp \
    portlist.cpp \
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
    remotelinuxprocesslist.cpp \
    remotelinuxprocessesdialog.cpp \
    remotelinuxdeploystepwidget.cpp \
    linuxdeviceconfigurationssettingswidget.cpp \
    remotelinuxenvironmentreader.cpp \
    sshkeydeployer.cpp \
    typespecificdeviceconfigurationlistmodel.cpp \
    sshkeycreationdialog.cpp \
    remotelinuxusedportsgatherer.cpp \
    remotelinuxsettingspages.cpp \
    remotelinuxutils.cpp \
    deploymentsettingsassistant.cpp \
    remotelinuxdeployconfigurationwidget.cpp \
    profilesupdatedialog.cpp \
    startgdbserverdialog.cpp \
    remotelinuxcustomcommanddeployservice.cpp \
    remotelinuxcustomcommanddeploymentstep.cpp

FORMS += \
    linuxdevicefactoryselectiondialog.ui \
    genericlinuxdeviceconfigurationwizardsetuppage.ui \
    linuxdevicetestdialog.ui \
    remotelinuxprocessesdialog.ui \
    linuxdeviceconfigurationssettingswidget.ui \
    sshkeycreationdialog.ui \
    remotelinuxdeployconfigurationwidget.ui \
    profilesupdatedialog.ui \
    startgdbserverdialog.ui

RESOURCES += remotelinux.qrc

DEFINES += QT_NO_CAST_TO_ASCII
DEFINES += REMOTELINUX_LIBRARY
