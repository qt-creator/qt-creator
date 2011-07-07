TEMPLATE = lib
TARGET = RemoteLinux

include(../../qtcreatorplugin.pri)
include(remotelinux_dependencies.pri)

HEADERS += \
    remotelinuxplugin.h \
    remotelinux_export.h \
    maemoconfigtestdialog.h \
    maemoconstants.h \
    maemorunconfigurationwidget.h \
    maemoruncontrol.h \
    maemorunfactories.h \
    maemosettingspages.h \
    maemodeviceconfigurationssettingswidget.h \
    maemosshconfigdialog.h \
    maemotoolchain.h \
    maemopackagecreationstep.h \
    maemopackagecreationfactory.h \
    maemopackagecreationwidget.h \
    maemoqemumanager.h \
    maemodeploystepwidget.h \
    maemodeploystepfactory.h \
    maemoglobal.h \
    maemosshrunner.h \
    maemodebugsupport.h \
    maemoremotemountsmodel.h \
    maemodeviceenvreader.h \
    maemomountspecification.h \
    maemoremotemounter.h \
    maemoprofilesupdatedialog.h \
    maemousedportsgatherer.h \
    maemoremoteprocesslist.h \
    maemoremoteprocessesdialog.h \
    maemopublishingwizardfactories.h \
    maemopublishingbuildsettingspagefremantlefree.h \
    maemopublishingfileselectiondialog.h \
    maemopublishedprojectmodel.h \
    maemopublishinguploadsettingspagefremantlefree.h \
    maemopublishingwizardfremantlefree.h \
    maemopublishingresultpagefremantlefree.h \
    maemopublisherfremantlefree.h \
    maemoqemuruntime.h \
    maemoqemuruntimeparser.h \
    maemoqemusettingswidget.h \
    maemoqemusettings.h \
    qt4maemotargetfactory.h \
    qt4maemotarget.h \
    qt4maemodeployconfiguration.h \
    maemodeviceconfigwizard.h \
    maemokeydeployer.h \
    maemopertargetdeviceconfigurationlistmodel.h \
    maemodeployconfigurationwidget.h \
    maemoinstalltosysrootstep.h \
    maemodeploymentmounter.h \
    maemopackageuploader.h \
    maemopackageinstaller.h \
    maemoremotecopyfacility.h \
    abstractmaemodeploystep.h \
    maemodeploybymountstep.h \
    maemouploadandinstalldeploystep.h \
    maemodirectdeviceuploadstep.h \
    abstractlinuxdevicedeploystep.h \
    maemoqtversionfactory.h \
    maemoqtversion.h \
    linuxdeviceconfiguration.h \
    linuxdeviceconfigurations.h \
    remotelinuxrunconfiguration.h \
    linuxdevicefactoryselectiondialog.h \
    publickeydeploymentdialog.h \
    genericlinuxdeviceconfigurationwizard.h \
    genericlinuxdeviceconfigurationfactory.h \
    maddedeviceconfigurationfactory.h \
    maemorunconfiguration.h \
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
    deploymentinfo.h

SOURCES += \
    remotelinuxplugin.cpp \
    maemoconfigtestdialog.cpp \
    maemorunconfigurationwidget.cpp \
    maemoruncontrol.cpp \
    maemorunfactories.cpp \
    maemosettingspages.cpp \
    maemodeviceconfigurationssettingswidget.cpp \
    maemosshconfigdialog.cpp \
    maemotoolchain.cpp \
    maemopackagecreationstep.cpp \
    maemopackagecreationfactory.cpp \
    maemopackagecreationwidget.cpp \
    maemoqemumanager.cpp \
    maemodeploystepwidget.cpp \
    maemodeploystepfactory.cpp \
    maemoglobal.cpp \
    maemosshrunner.cpp \
    maemodebugsupport.cpp \
    maemoremotemountsmodel.cpp \
    maemodeviceenvreader.cpp \
    maemomountspecification.cpp \
    maemoremotemounter.cpp \
    maemoprofilesupdatedialog.cpp \
    maemousedportsgatherer.cpp \
    maemoremoteprocesslist.cpp \
    maemoremoteprocessesdialog.cpp \
    maemopublishingwizardfactories.cpp \
    maemopublishingbuildsettingspagefremantlefree.cpp \
    maemopublishingfileselectiondialog.cpp \
    maemopublishedprojectmodel.cpp \
    maemopublishinguploadsettingspagefremantlefree.cpp \
    maemopublishingwizardfremantlefree.cpp \
    maemopublishingresultpagefremantlefree.cpp \
    maemopublisherfremantlefree.cpp \
    maemoqemuruntimeparser.cpp \
    maemoqemusettingswidget.cpp \
    maemoqemusettings.cpp \
    qt4maemotargetfactory.cpp \
    qt4maemotarget.cpp \
    qt4maemodeployconfiguration.cpp \
    maemodeviceconfigwizard.cpp \
    maemokeydeployer.cpp \
    maemopertargetdeviceconfigurationlistmodel.cpp \
    maemodeployconfigurationwidget.cpp \
    maemoinstalltosysrootstep.cpp \
    maemodeploymentmounter.cpp \
    maemopackageuploader.cpp \
    maemopackageinstaller.cpp \
    maemoremotecopyfacility.cpp \
    abstractmaemodeploystep.cpp \
    maemodeploybymountstep.cpp \
    maemouploadandinstalldeploystep.cpp \
    maemodirectdeviceuploadstep.cpp \
    abstractlinuxdevicedeploystep.cpp \
    maemoqtversionfactory.cpp \
    maemoqtversion.cpp \
    linuxdeviceconfiguration.cpp \
    linuxdeviceconfigurations.cpp \
    remotelinuxrunconfiguration.cpp \
    linuxdevicefactoryselectiondialog.cpp \
    publickeydeploymentdialog.cpp \
    genericlinuxdeviceconfigurationwizard.cpp \
    genericlinuxdeviceconfigurationfactory.cpp \
    maddedeviceconfigurationfactory.cpp \
    maemorunconfiguration.cpp \
    remotelinuxrunconfigurationwidget.cpp \
    remotelinuxrunconfigurationfactory.cpp \
    remotelinuxapplicationrunner.cpp \
    remotelinuxruncontrol.cpp \
    remotelinuxruncontrolfactory.cpp \
    remotelinuxdebugsupport.cpp \
    genericlinuxdeviceconfigurationwizardpages.cpp \
    portlist.cpp \
    deployablefilesperprofile.cpp \
    deploymentinfo.cpp

FORMS += \
    maemoconfigtestdialog.ui \
    maemodeviceconfigurationssettingswidget.ui \
    maemosshconfigdialog.ui \
    maemopackagecreationwidget.ui \
    maemoprofilesupdatedialog.ui \
    maemoremoteprocessesdialog.ui \
    maemopublishingbuildsettingspagefremantlefree.ui \
    maemopublishingfileselectiondialog.ui \
    maemopublishinguploadsettingspagefremantlefree.ui \
    maemopublishingresultpagefremantlefree.ui \
    maemoqemusettingswidget.ui \
    maemodeviceconfigwizardstartpage.ui \
    maemodeviceconfigwizardpreviouskeysetupcheckpage.ui \
    maemodeviceconfigwizardreusekeyscheckpage.ui \
    maemodeviceconfigwizardkeycreationpage.ui \
    maemodeviceconfigwizardkeydeploymentpage.ui \
    maemodeployconfigurationwidget.ui \
    linuxdevicefactoryselectiondialog.ui \
    genericlinuxdeviceconfigurationwizardsetuppage.ui

RESOURCES += qt-maemo.qrc
DEFINES += QT_NO_CAST_TO_ASCII
DEFINES += REMOTELINUX_LIBRARY
