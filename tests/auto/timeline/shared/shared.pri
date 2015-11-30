QT += quick

QTC_LIB_DEPENDS += timeline
include(../../qttest.pri)

SOURCES += \
    $$PWD/runscenegraph.cpp
HEADERS += \
    $$PWD/runscenegraph.h
