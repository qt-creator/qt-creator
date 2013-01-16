include(../../auto/qttest.pri)
include($$IDE_SOURCE_TREE/src/plugins/valgrind/valgrind_test.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)
include($$IDE_SOURCE_TREE/src/libs/ssh/ssh.pri)
TARGET = callgrindparsertests

DEFINES += "PARSERTESTS_DATA_DIR=\\\"$$PWD/data\\\""
#enable extra debugging code
DEFINES += "CALLGRINDPARSERTESTS"

SOURCES += callgrindparsertests.cpp
HEADERS += callgrindparsertests.h
