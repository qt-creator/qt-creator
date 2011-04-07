include(../../../qtcreator.pri)
TEMPLATE = app
TARGET = parsertests

macx:CONFIG -= app_bundle

QT += testlib network

DEFINES += "PARSERTESTS_DATA_DIR=\\\"$$PWD/data\\\""

DEFINES += "VALGRIND_FAKE_PATH=\\\"$$IDE_BUILD_TREE/src/tools/valgrindfake/valgrind-fake\\\""

!win32 {
    include($$IDE_SOURCE_TREE/src/libs/valgrind/valgrind.pri)
    include($$IDE_SOURCE_TREE/src/libs/3rdparty/botan/botan.pri)
    include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)
}

SOURCES += parsertests.cpp

HEADERS += parsertests.h
