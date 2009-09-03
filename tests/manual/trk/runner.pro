
TEMPLATE = app

QT += network

win32:CONFIG+=console

HEADERS += \
    trkutils.h \
    trkdevice.h \

SOURCES += \
    runner.cpp \
    trkutils.cpp \
    trkdevice.cpp \
