
TEMPLATE = app

TRK_DIR=../../../src/shared/trk
include($$TRK_DIR/trk.pri)

INCLUDEPATH *= $$TRK_DIR

UTILSDIR = ../../../src/libs
QT = core network
win32:CONFIG+=console

HEADERS += \
    trkolddevice.h
SOURCES += \
    trkolddevice.cpp \
    adapter.cpp
