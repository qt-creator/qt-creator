TEMPLATE = app
TARGET = callgrindparsertests

macx:CONFIG -= app_bundle

QT += testlib network

DEFINES += "PARSERTESTS_DATA_DIR=\"\\\"$$PWD/data\\\"\""
#enable extra debugging code
DEFINES += "CALLGRINDPARSERTESTS"

!win32 {
    include(../../../qtcreator.pri)
    include(../../../src/libs/valgrind/valgrind.pri)
}

SOURCES += callgrindparsertests.cpp

HEADERS += callgrindparsertests.h
