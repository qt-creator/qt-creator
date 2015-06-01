QT       += core network

QT       -= gui

TARGET = echo
CONFIG   += console c++14
CONFIG   -= app_bundle

TEMPLATE = app

unix:LIBS += -ldl

osx:QMAKE_CXXFLAGS = -stdlib=libc++

include(../../../../src/libs/codemodelbackendipc/codemodelbackendipc-lib.pri)
include(../../../../src/libs/sqlite/sqlite-lib.pri)

SOURCES += \
    echoipcserver.cpp \
    echoserverprocessmain.cpp

HEADERS += \
    echoipcserver.h

DEFINES += CODEMODELBACKENDIPC_TESTS
DEFINES += DONT_CHECK_COMMAND_COUNTER

win32:DESTDIR = ..
