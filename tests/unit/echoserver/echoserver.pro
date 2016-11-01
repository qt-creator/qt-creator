QT       += core network

TARGET = echo
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

unix:LIBS += -ldl

osx:QMAKE_CXXFLAGS = -stdlib=libc++

include(../../../src/libs/utils/utils-lib.pri)
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
