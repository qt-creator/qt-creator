TEMPLATE = lib
TARGET = Ios

include(../../qtcreatorplugin.pri)

QT += xml network

macx: LIBS += -framework CoreFoundation -framework IOKit

HEADERS += \
    iosconstants.h \
    iosconfigurations.h \
    iosmanager.h \
    iosrunconfiguration.h \
    iosrunfactories.h \
    iossettingspage.h \
    iossettingswidget.h \
    iosrunner.h \
    iosdsymbuildstep.h \
    iosqtversionfactory.h \
    iosqtversion.h \
    iosplugin.h \
    iosdevicefactory.h \
    iosdevice.h \
    iossimulator.h \
    iossimulatorfactory.h \
    iosprobe.h \
    iosbuildstep.h \
    iostoolhandler.h \
    iosdeployconfiguration.h \
    iosdeploystep.h \
    iosdeploystepfactory.h \
    iosdeploystepwidget.h \
    simulatorcontrol.h \
    iosbuildconfiguration.h \
    iosbuildsettingswidget.h \
    createsimulatordialog.h \
    simulatoroperationdialog.h \
    simulatorinfomodel.h


SOURCES += \
    iosconfigurations.cpp \
    iosmanager.cpp \
    iosrunconfiguration.cpp \
    iosrunfactories.cpp \
    iossettingspage.cpp \
    iossettingswidget.cpp \
    iosrunner.cpp \
    iosdsymbuildstep.cpp \
    iosqtversionfactory.cpp \
    iosqtversion.cpp \
    iosplugin.cpp \
    iosdevicefactory.cpp \
    iosdevice.cpp \
    iossimulator.cpp \
    iossimulatorfactory.cpp \
    iosprobe.cpp \
    iosbuildstep.cpp \
    iostoolhandler.cpp \
    iosdeployconfiguration.cpp \
    iosdeploystep.cpp \
    iosdeploystepfactory.cpp \
    iosdeploystepwidget.cpp \
    simulatorcontrol.cpp \
    iosbuildconfiguration.cpp \
    iosbuildsettingswidget.cpp \
    createsimulatordialog.cpp \
    simulatoroperationdialog.cpp \
    simulatorinfomodel.cpp

FORMS += \
    iossettingswidget.ui \
    iosbuildstep.ui \
    iosdeploystepwidget.ui \
    iospresetbuildstep.ui \
    iosbuildsettingswidget.ui \
    createsimulatordialog.ui \
    simulatoroperationdialog.ui

DEFINES += IOS_LIBRARY

RESOURCES += ios.qrc

win32: LIBS += -lws2_32
