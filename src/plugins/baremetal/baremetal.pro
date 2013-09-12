QT += network
PROVIDER = Sander
include(../../qtcreatorplugin.pri)

# BareMetal files

SOURCES += baremetalplugin.cpp \
    baremetaldevice.cpp \
    baremetaldeployconfigurationfactory.cpp \
    baremetaldeployconfiguration.cpp \
    baremetalrunconfigurationfactory.cpp \
    baremetalrunconfiguration.cpp \
    baremetalrunconfigurationwidget.cpp \
    baremetaldeploystepfactory.cpp \
    baremetalgdbcommandsdeploystep.cpp \
    baremetalruncontrolfactory.cpp \
    baremetaldeviceconfigurationwizardpages.cpp \
    baremetaldeviceconfigurationwizard.cpp \
    baremetaldeviceconfigurationwidget.cpp \
    baremetaldeviceconfigurationfactory.cpp

HEADERS += baremetalplugin.h \
    baremetalconstants.h \
    baremetaldevice.h \
    baremetaldeployconfigurationfactory.h \
    baremetaldeployconfiguration.h \
    baremetalrunconfigurationfactory.h \
    baremetalrunconfiguration.h \
    baremetalrunconfigurationwidget.h \
    baremetaldeploystepfactory.h \
    baremetalgdbcommandsdeploystep.h \
    baremetalruncontrolfactory.h \
    baremetaldeviceconfigurationfactory.h \
    baremetaldeviceconfigurationwidget.h \
    baremetaldeviceconfigurationwizard.h \
    baremetaldeviceconfigurationwizardpages.h

FORMS += \
    baremetaldeviceconfigurationwizardsetuppage.ui \
    baremetaldeviceconfigurationwidget.ui
