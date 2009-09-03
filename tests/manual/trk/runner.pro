
TEMPLATE = app

QT += network

win32:CONFIG+=console

HEADERS += \
    trkutils.h \
    trkdevicex.h \

SOURCES += \
    runner.cpp \
    trkutils.cpp \
    trkdevicex.cpp \
