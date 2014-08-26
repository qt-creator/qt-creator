TARGET = pluginmanager

# Input

QTC_LIB_DEPENDS += extensionsystem
include(../../qttest.pri)

SOURCES += tst_pluginmanager.cpp

DEFINES += "PLUGINMANAGER_TESTS_DIR=\\\"$$OUT_PWD\\\""
