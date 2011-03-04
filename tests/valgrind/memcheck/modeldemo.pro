TEMPLATE = app
TARGET = modeldemo

macx:CONFIG -= app_bundle

QT += gui network

DEFINES += "PARSERTESTS_DATA_DIR=\"\\\"$$PWD/data\\\"\""

!win32 {
    include(../../../qtcreator.pri)
    include(../../../src/libs/valgrind/valgrind.pri)
}

SOURCES += modeldemo.cpp

HEADERS += modeldemo.h
