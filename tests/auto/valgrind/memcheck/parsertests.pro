QTC_LIB_DEPENDS += utils ssh
QTC_PLUGIN_DEPENDS += projectexplorer
include(../../qttest.pri)
include($$IDE_SOURCE_TREE/src/plugins/valgrind/valgrind_test.pri)

TARGET = tst_parsertests

QT += network

DEFINES += "PARSERTESTS_DATA_DIR=\\\"$$_PRO_FILE_PWD_/data\\\""
DEFINES += "VALGRIND_FAKE_PATH=\\\"$$IDE_BUILD_TREE/src/tools/valgrindfake/valgrind-fake\\\""

SOURCES += parsertests.cpp
HEADERS += parsertests.h
