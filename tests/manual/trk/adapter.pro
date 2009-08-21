
TEMPLATE = app

QT = core network
win32:CONFIG+=console

HEADERS += trkutils.h \
trkfunctor.h \
trkdevice.h \

SOURCES += \
    adapter.cpp \
    trkutils.cpp \
    trkdevice.cpp
