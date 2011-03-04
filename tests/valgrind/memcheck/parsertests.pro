TEMPLATE = app
TARGET = parsertests

macx:CONFIG -= app_bundle

QT += testlib network

DEFINES += "PARSERTESTS_DATA_DIR=\"\\\"$$PWD/data\\\"\""

DEFINES += "VALGRIND_FAKE_PATH="\\\"$$IDE_BUILD_TREE/src/tools/valgrindfake/valgrind-fake\\\""

!win32 {
    include(../../../qtcreator.pri)
    include(../../../src/libs/valgrind/valgrind.pri)
}

SOURCES += parsertests.cpp

HEADERS += parsertests.h
