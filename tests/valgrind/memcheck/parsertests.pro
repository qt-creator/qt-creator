include(../../auto/qttest.pri)
include($$IDE_SOURCE_TREE/src/libs/3rdparty/botan/botan.pri)
include($$IDE_SOURCE_TREE/src/libs/ssh/ssh.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)
include($$IDE_SOURCE_TREE/src/plugins/valgrind/valgrind_test.pri)

TARGET = parsertests

QT += network

DEFINES += "PARSERTESTS_DATA_DIR=\\\"$$PWD/data\\\""
DEFINES += "VALGRIND_FAKE_PATH=\\\"$$IDE_BUILD_TREE/src/tools/valgrindfake/valgrind-fake\\\""

SOURCES += parsertests.cpp
HEADERS += parsertests.h
