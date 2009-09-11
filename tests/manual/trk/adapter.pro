
TEMPLATE = app

DEBUGGERHOME = ../../../src/plugins/debugger/gdb

INCLUDEPATH *= $$DEBUGGERHOME

UTILSDIR = ../../../src/libs
QT = core network
win32:CONFIG+=console

HEADERS += \
    $$DEBUGGERHOME/trkutils.h \
    $$DEBUGGERHOME/callback.h \
    $$PWD/trkdevice.h \

SOURCES += \
    $$DEBUGGERHOME/trkutils.cpp \
    $$PWD/trkdevice.cpp \
    $$PWD/adapter.cpp \
