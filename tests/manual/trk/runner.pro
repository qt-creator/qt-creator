
TEMPLATE = app

DEBUGGERHOME = ../../../src/plugins/debugger/gdb
INCLUDEPATH *= $$DEBUGGERHOME

DEFINES += STANDALONE_RUNNER

QT += network

win32:CONFIG+=console

HEADERS += \
    $$DEBUGGERHOME/../gdb/abstractgdbadapter.h \
    $$DEBUGGERHOME/trkutils.h \
    $$DEBUGGERHOME/trkclient.h \
    $$DEBUGGERHOME/trkgdbadapter.h \

SOURCES += \
    $$DEBUGGERHOME/trkutils.cpp \
    $$DEBUGGERHOME/trkclient.cpp \
    $$DEBUGGERHOME/trkgdbadapter.cpp \
    $$PWD/runner.cpp \
