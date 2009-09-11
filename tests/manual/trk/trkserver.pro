
TEMPLATE = app

DEBUGGERHOME = ../../../src/plugins/debugger/gdb

QT = core network
win32:CONFIG+=console

INCLUDEPATH *= $$DEBUGGERHOME


HEADERS += \
    $$DEBUGGERHOME/trkutils.h

SOURCES += \
    $$DEBUGGERHOME/trkutils.cpp \
    $$PWD/trkserver.cpp
