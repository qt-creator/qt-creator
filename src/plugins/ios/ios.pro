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
    iosruncontrol.h \
    iosrunfactories.h \
    iossettingspage.h \
    iossettingswidget.h \
    iosrunner.h \
    iosdebugsupport.h \
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
    iosanalyzesupport.h \
    simulatorcontrol.h


SOURCES += \
    iosconfigurations.cpp \
    iosmanager.cpp \
    iosrunconfiguration.cpp \
    iosruncontrol.cpp \
    iosrunfactories.cpp \
    iossettingspage.cpp \
    iossettingswidget.cpp \
    iosrunner.cpp \
    iosdebugsupport.cpp \
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
    iosanalyzesupport.cpp \
    simulatorcontrol.cpp

FORMS += \
    iossettingswidget.ui \
    iosbuildstep.ui \
    iosdeploystepwidget.ui \
    iospresetbuildstep.ui

DEFINES += IOS_LIBRARY

RESOURCES += ios.qrc

win32: LIBS += -lws2_32
