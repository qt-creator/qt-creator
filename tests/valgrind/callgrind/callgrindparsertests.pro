include(../../auto/qttest.pri)
include(../../../src/libs/valgrind/valgrind.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)
TARGET = callgrindparsertests

DEFINES += "PARSERTESTS_DATA_DIR=\\\"$$PWD/data\\\""
#enable extra debugging code
DEFINES += "CALLGRINDPARSERTESTS"

SOURCES += callgrindparsertests.cpp
HEADERS += callgrindparsertests.h
