
TEMPLATE = app

DEBUGGERHOME = ../../../src/plugins/debugger/gdb
INCLUDEPATH *= $$DEBUGGERHOME

DEFINES += STANDALONE_RUNNER

QT += network

win32:CONFIG+=console

HEADERS += \
    $$DEBUGGERHOME/../gdb/gdbprocessbase.h \
    $$DEBUGGERHOME/trkutils.h \
    $$DEBUGGERHOME/trkclient.h \
    $$DEBUGGERHOME/symbianadapter.h \

SOURCES += \
    $$DEBUGGERHOME/trkutils.cpp \
    $$DEBUGGERHOME/trkclient.cpp \
    $$DEBUGGERHOME/symbianadapter.cpp \
    $$PWD/runner.cpp \
