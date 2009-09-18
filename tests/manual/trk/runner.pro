
TEMPLATE = app

TRK_DIR=../../../src/shared/trk
include($$TRK_DIR/trk.pri)
DEBUGGERHOME = ../../../src/plugins/debugger/gdb
INCLUDEPATH *= $$TRK_DIR
INCLUDEPATH *= $$DEBUGGERHOME

DEFINES += STANDALONE_RUNNER

QT += network

win32:CONFIG+=console

HEADERS += \
    $$DEBUGGERHOME/abstractgdbadapter.h \
    $$DEBUGGERHOME/trkoptions.h \
    $$DEBUGGERHOME/trkgdbadapter.h \

SOURCES += \
    $$DEBUGGERHOME/trkgdbadapter.cpp \
    $$DEBUGGERHOME/trkoptions.cpp \
    $$PWD/runner.cpp \
