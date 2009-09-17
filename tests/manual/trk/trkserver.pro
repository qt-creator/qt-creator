
TEMPLATE = app

include(../../../src/shared/trk/trk.pri)

QT = core network
win32:CONFIG+=console

SOURCES += \
    $$PWD/trkserver.cpp
