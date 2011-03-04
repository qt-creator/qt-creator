TEMPLATE = app
TARGET = testrunner

macx:CONFIG -= app_bundle

QT += testlib network

DEFINES += "TESTRUNNER_SRC_DIR=\"\\\"$$_PRO_FILE_PWD_/testapps\\\"\""
DEFINES += "TESTRUNNER_APP_DIR=\"\\\"$(PWD)/testapps\\\"\""

!win32 {
    include(../../../qtcreator.pri)
    include(../../../src/libs/valgrind/valgrind.pri)
}

SOURCES += testrunner.cpp

HEADERS += testrunner.h
