TEMPLATE = lib
TARGET = Ios

include(../../qtcreatorplugin.pri)

QT += xml network

macx: LIBS += -framework CoreFoundation -framework IOKit

HEADERS += \
    iosconstants.h \
    iosconfigurations.h \
    iosrunconfiguration.h \
    iossettingspage.h \
    iossettingswidget.h \
    iosrunner.h \
    iosdsymbuildstep.h \
    iosqtversion.h \
    iosplugin.h \
    iosdevice.h \
    iossimulator.h \
    iosprobe.h \
    iosbuildstep.h \
    iostoolhandler.h \
    iosdeploystep.h \
    simulatorcontrol.h \
    iosbuildconfiguration.h \
    createsimulatordialog.h \
    simulatoroperationdialog.h \
    simulatorinfomodel.h


SOURCES += \
    iosconfigurations.cpp \
    iosrunconfiguration.cpp \
    iossettingspage.cpp \
    iossettingswidget.cpp \
    iosrunner.cpp \
    iosdsymbuildstep.cpp \
    iosqtversion.cpp \
    iosplugin.cpp \
    iosdevice.cpp \
    iossimulator.cpp \
    iosprobe.cpp \
    iosbuildstep.cpp \
    iostoolhandler.cpp \
    iosdeploystep.cpp \
    simulatorcontrol.cpp \
    iosbuildconfiguration.cpp \
    createsimulatordialog.cpp \
    simulatoroperationdialog.cpp \
    simulatorinfomodel.cpp

FORMS += \
    iossettingswidget.ui \
    iospresetbuildstep.ui \
    createsimulatordialog.ui \
    simulatoroperationdialog.ui

DEFINES += IOS_LIBRARY

RESOURCES += ios.qrc

win32: LIBS += -lws2_32
