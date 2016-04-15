QT       += core network

QT       -= gui

TARGET = echo
CONFIG   += console c++11
CONFIG   -= app_bundle

TEMPLATE = app

unix:LIBS += -ldl

osx:QMAKE_CXXFLAGS = -stdlib=libc++

include(../../../src/libs/clangbackendipc/clangbackendipc-lib.pri)
include(../../../src/libs/sqlite/sqlite-lib.pri)

SOURCES += \
    echoipcserver.cpp \
    echoserverprocessmain.cpp

HEADERS += \
    echoipcserver.h

DEFINES += CLANGBACKENDIPC_TESTS
DEFINES += DONT_CHECK_MESSAGE_COUNTER

win32:DESTDIR = ..
