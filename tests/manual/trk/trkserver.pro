
TEMPLATE = app

DEBUGGERHOME = ../../../src/plugins/debugger/symbian

QT = core network
win32:CONFIG+=console

INCLUDEPATH *= $$DEBUGGERHOME


HEADERS += \
    $$DEBUGGERHOME/trkutils.h

SOURCES += \
    $$DEBUGGERHOME/trkutils.cpp \
    $$PWD/trkserver.cpp
