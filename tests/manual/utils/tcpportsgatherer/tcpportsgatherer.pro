TEMPLATE = app
TARGET = tcpportsgatherer

QT = core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += console
CONFIG -= app_bundle

include(../../../auto/qttest.pri)
include(../../../../src/rpath.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)


INCLUDEPATH += ../../../../src/libs
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
