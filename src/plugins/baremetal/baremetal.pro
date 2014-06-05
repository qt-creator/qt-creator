QT += network
include(../../qtcreatorplugin.pri)

# BareMetal files

SOURCES += baremetalplugin.cpp \
    baremetaldevice.cpp \
    baremetalrunconfigurationfactory.cpp \
    baremetalrunconfiguration.cpp \
    baremetalrunconfigurationwidget.cpp \
    baremetalgdbcommandsdeploystep.cpp \
    baremetalruncontrolfactory.cpp \
    baremetaldeviceconfigurationwizardpages.cpp \
    baremetaldeviceconfigurationwizard.cpp \
    baremetaldeviceconfigurationwidget.cpp \
    baremetaldeviceconfigurationfactory.cpp

HEADERS += baremetalplugin.h \
    baremetalconstants.h \
    baremetaldevice.h \
    baremetalrunconfigurationfactory.h \
    baremetalrunconfiguration.h \
    baremetalrunconfigurationwidget.h \
    baremetalgdbcommandsdeploystep.h \
    baremetalruncontrolfactory.h \
    baremetaldeviceconfigurationfactory.h \
    baremetaldeviceconfigurationwidget.h \
    baremetaldeviceconfigurationwizard.h \
    baremetaldeviceconfigurationwizardpages.h
