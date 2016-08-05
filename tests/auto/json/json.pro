TARGET = tst_json
QT = core testlib
CONFIG -= app_bundle
CONFIG += testcase

TESTDATA += test.json test.bjson test3.json test2.json bom.json

INCLUDEPATH += ../../../src/shared/json

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

include(../../../src/shared/json/json.pri)
SOURCES += tst_json.cpp
