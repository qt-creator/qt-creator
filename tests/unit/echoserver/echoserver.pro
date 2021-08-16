QT       += core network

TARGET = echo
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

unix:LIBS += -ldl

include($$PWD/../../../src/libs/utils/utils-lib.pri)

include(../../../qtcreator.pri)
include(../../../src/libs/clangsupport/clangsupport-lib.pri)
include(../../../src/libs/sqlite/sqlite-lib.pri)

INCLUDEPATH += ../../../src/libs

SOURCES += \
    echoclangcodemodelserver.cpp \
    echoserverprocessmain.cpp

HEADERS += \
    echoclangcodemodelserver.h

DEFINES += CLANGSUPPORT_TESTS
DEFINES += DONT_CHECK_MESSAGE_COUNTER

win32:DESTDIR = ..
