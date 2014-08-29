TEMPLATE = app
TARGET = tcpportsgatherer

QT = core widgets network

CONFIG += console
CONFIG -= app_bundle

QTC_LIB_DEPENDS += utils
include(../../../auto/qttest.pri)
include(../../../../src/rpath.pri)


UTILSDIR = ../../../../src/libs/utils

DEFINES += QTCREATOR_UTILS_STATIC_LIB

HEADERS += \
    $${UTILSDIR}/portlist.h \
    $${UTILSDIR}/tcpportsgatherer.h
SOURCES += \
    $${UTILSDIR}/portlist.cpp \
    $${UTILSDIR}/tcpportsgatherer.cpp

win32:LIBS += -liphlpapi -lws2_32
SOURCES += main.cpp
