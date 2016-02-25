QTC_LIB_DEPENDS += utils ssh
QTC_PLUGIN_DEPENDS += debugger projectexplorer
include(../../qttest.pri)
include($$IDE_SOURCE_TREE/src/plugins/valgrind/valgrind_test.pri)

TARGET = tst_testrunner

DEFINES += "TESTRUNNER_SRC_DIR=\\\"$$_PRO_FILE_PWD_/testapps\\\""
DEFINES += "TESTRUNNER_APP_DIR=\\\"$(PWD)/testapps\\\""

SOURCES += testrunner.cpp
HEADERS += testrunner.h
