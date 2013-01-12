TEMPLATE = lib
TARGET = Madde
QT += network

include(../../qtcreatorplugin.pri)
include(madde_dependencies.pri)

HEADERS += \
    madde_exports.h \
    maddeplugin.h \
    debianmanager.h \
    maemoconstants.h \
    maemorunconfigurationwidget.h \
    maemorunfactories.h \
    maemosettingspages.h \
    maemopackagecreationstep.h \
    maemopackagecreationfactory.h \
    maemopackagecreationwidget.h \
    maemoqemumanager.h \
    maemodeploystepfactory.h \
    maemoglobal.h \
    maemoremotemountsmodel.h \
    maemomountspecification.h \
    maemoremotemounter.h \
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
    qt4maemodeployconfiguration.h \
    maemodeviceconfigwizard.h \
    maemoinstalltosysrootstep.h \
    maemodeploymentmounter.h \
    maemopackageinstaller.h \
    maemoremotecopyfacility.h \
    maemoqtversionfactory.h \
    maemoqtversion.h \
    maemorunconfiguration.h \
    maddeuploadandinstallpackagesteps.h \
    maemodeploybymountsteps.h \
    maddedevicetester.h \
    maddedeviceconfigurationfactory.h \
    maddedevice.h \
    maemoapplicationrunnerhelperactions.h \
    maddeqemustartstep.h

SOURCES += \
    maddeplugin.cpp \
    debianmanager.cpp \
    maemorunconfigurationwidget.cpp \
    maemorunfactories.cpp \
    maemosettingspages.cpp \
    maemopackagecreationstep.cpp \
    maemopackagecreationfactory.cpp \
    maemopackagecreationwidget.cpp \
    maemoqemumanager.cpp \
    maemodeploystepfactory.cpp \
    maemoglobal.cpp \
    maemoremotemountsmodel.cpp \
    maemomountspecification.cpp \
    maemoremotemounter.cpp \
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
    qt4maemodeployconfiguration.cpp \
    maemodeviceconfigwizard.cpp \
    maemoinstalltosysrootstep.cpp \
    maemodeploymentmounter.cpp \
    maemopackageinstaller.cpp \
    maemoremotecopyfacility.cpp \
    maemoqtversionfactory.cpp \
    maemoqtversion.cpp \
    maddedeviceconfigurationfactory.cpp \
    maddeuploadandinstallpackagesteps.cpp \
    maemodeploybymountsteps.cpp \
    maddedevicetester.cpp \
    maemorunconfiguration.cpp \
    maddedevice.cpp \
    maemoapplicationrunnerhelperactions.cpp \
    maddeqemustartstep.cpp

FORMS += \
    maemopackagecreationwidget.ui \
    maemopublishingbuildsettingspagefremantlefree.ui \
    maemopublishingfileselectiondialog.ui \
    maemopublishinguploadsettingspagefremantlefree.ui \
    maemopublishingresultpagefremantlefree.ui \
    maemoqemusettingswidget.ui \
    maemodeviceconfigwizardstartpage.ui \
    maemodeviceconfigwizardpreviouskeysetupcheckpage.ui \
    maemodeviceconfigwizardreusekeyscheckpage.ui \
    maemodeviceconfigwizardkeycreationpage.ui \
    maemodeviceconfigwizardkeydeploymentpage.ui

RESOURCES += qt-maemo.qrc
DEFINES += MADDE_LIBRARY
