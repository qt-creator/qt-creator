
TEMPLATE = app

QT = core network
win32:CONFIG+=console

HEADERS += \
    trkutils.h

SOURCES += \
    trkutils.cpp \
    trkserver.cpp
