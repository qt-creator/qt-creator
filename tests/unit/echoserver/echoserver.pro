QT       += core network

TARGET = echo
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

unix:LIBS += -ldl

include(../../../qtcreator.pri)
include(../../../src/libs/clangbackendipc/clangbackendipc-lib.pri)
include(../../../src/libs/sqlite/sqlite-lib.pri)

INCLUDEPATH += ../../../src/libs

SOURCES += \
    echoclangcodemodelserver.cpp \
    echoserverprocessmain.cpp

HEADERS += \
    echoclangcodemodelserver.h

DEFINES += CLANGBACKENDIPC_TESTS
DEFINES += DONT_CHECK_MESSAGE_COUNTER

win32:DESTDIR = ..
