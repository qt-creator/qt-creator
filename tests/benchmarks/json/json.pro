TARGET = tst_bench_json
QT = core testlib
CONFIG -= app_bundle

SOURCES += tst_bench_json.cpp

TESTDATA = numbers.json test.json


INCLUDEPATH += ../../../src/shared/json

#DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

include(../../../src/shared/json/json.pri)
