TEMPLATE = lib
TARGET = Qnx
QT += network xml
PROVIDER = RIM

include(../../qtcreatorplugin.pri)
include(qnx_dependencies.pri)

SOURCES += qnxplugin.cpp \
    blackberryqtversionfactory.cpp \
    blackberryqtversion.cpp \
    qnxbaseqtconfigwidget.cpp \
    blackberrydeployconfigurationfactory.cpp \
    blackberrydeployconfiguration.cpp \
    blackberrycreatepackagestep.cpp \
    blackberrycreatepackagestepconfigwidget.cpp \
    blackberrycreatepackagestepfactory.cpp \
    blackberrydeploystep.cpp \
    blackberrydeployconfigurationwidget.cpp \
    blackberrydeploystepconfigwidget.cpp \
    blackberrydeviceconfigurationfactory.cpp \
    blackberrydeviceconfigurationwizard.cpp \
    blackberrydeviceconfigurationwizardpages.cpp \
    blackberrydeploystepfactory.cpp \
    blackberryrunconfiguration.cpp \
    blackberryrunconfigurationwidget.cpp \
    blackberryrunconfigurationfactory.cpp \
    blackberryruncontrolfactory.cpp \
    blackberryruncontrol.cpp \
    blackberrydebugsupport.cpp \
    blackberryapplicationrunner.cpp \
    blackberryconnect.cpp \
    qnxutils.cpp \
    blackberrydeviceconfigurationwidget.cpp \
    qnxdeviceconfigurationfactory.cpp \
    qnxdeviceconfigurationwizard.cpp \
    qnxdeviceconfigurationwizardpages.cpp \
    qnxrunconfiguration.cpp \
    qnxruncontrolfactory.cpp \
    qnxdebugsupport.cpp \
    qnxapplicationrunner.cpp \
    qnxdeploystepfactory.cpp \
    qnxdeployconfigurationfactory.cpp \
    qnxrunconfigurationfactory.cpp \
    qnxruncontrol.cpp \
    qnxqtversionfactory.cpp \
    qnxqtversion.cpp \
    qnxabstractqtversion.cpp \
    bardescriptorfileimagewizardpage.cpp \
    blackberrywizardextension.cpp \
    blackberrydeviceconfiguration.cpp \
    qnxdeployconfiguration.cpp \
    qnxdeviceconfiguration.cpp \
    blackberrydeployinformation.cpp \
    pathchooserdelegate.cpp \
    blackberryabstractdeploystep.cpp

HEADERS += qnxplugin.h\
    qnxconstants.h \
    blackberryqtversionfactory.h \
    blackberryqtversion.h \
    qnxbaseqtconfigwidget.h \
    blackberrydeployconfigurationfactory.h \
    blackberrydeployconfiguration.h \
    blackberrycreatepackagestep.h \
    blackberrycreatepackagestepconfigwidget.h \
    blackberrycreatepackagestepfactory.h \
    blackberrydeploystep.h \
    blackberrydeployconfigurationwidget.h \
    blackberrydeploystepconfigwidget.h \
    blackberrydeviceconfigurationfactory.h \
    blackberrydeviceconfigurationwizard.h \
    blackberrydeviceconfigurationwizardpages.h \
    blackberrydeploystepfactory.h \
    blackberryrunconfiguration.h \
    blackberryrunconfigurationwidget.h \
    blackberryrunconfigurationfactory.h \
    blackberryruncontrolfactory.h \
    blackberryruncontrol.h \
    blackberrydebugsupport.h \
    blackberryapplicationrunner.h \
    blackberryconnect.h \
    qnxutils.h \
    blackberrydeviceconfigurationwidget.h \
    qnxdeviceconfigurationfactory.h \
    qnxdeviceconfigurationwizard.h \
    qnxdeviceconfigurationwizardpages.h \
    qnxrunconfiguration.h \
    qnxruncontrolfactory.h \
    qnxdebugsupport.h \
    qnxapplicationrunner.h \
    qnxdeploystepfactory.h \
    qnxdeployconfigurationfactory.h \
    qnxrunconfigurationfactory.h \
    qnxruncontrol.h \
    qnxqtversionfactory.h \
    qnxqtversion.h \
    qnxabstractqtversion.h \
    bardescriptorfileimagewizardpage.h \
    blackberrywizardextension.h \
    blackberrydeviceconfiguration.h \
    qnxdeployconfiguration.h \
    qnxdeviceconfiguration.h \
    blackberrydeployinformation.h \
    pathchooserdelegate.h \
    blackberryabstractdeploystep.h

FORMS += \
    blackberrydeviceconfigurationwizardsetuppage.ui \
    blackberryrunconfigurationwidget.ui \
    blackberrydeviceconfigurationwizardsshkeypage.ui \
    blackberrydeployconfigurationwidget.ui \
    blackberrydeviceconfigurationwidget.ui \
    qnxbaseqtconfigwidget.ui \
    bardescriptorfileimagewizardpage.ui

DEFINES += QT_NO_CAST_TO_ASCII

RESOURCES += \
    qnx.qrc
