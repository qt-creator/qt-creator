include(../../../qtcreator.pri)
TEMPLATE = app
TARGET = testrunner

macx:CONFIG -= app_bundle

QT += testlib network

DEFINES += "TESTRUNNER_SRC_DIR=\\\"$$_PRO_FILE_PWD_/testapps\\\""
DEFINES += "TESTRUNNER_APP_DIR=\\\"$(PWD)/testapps\\\""

!win32 {
    include($$IDE_SOURCE_TREE/src/libs/valgrind/valgrind.pri)
    include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)
    include($$IDE_SOURCE_TREE/src/libs/3rdparty/botan/botan.pri)
}

SOURCES += testrunner.cpp

HEADERS += testrunner.h
