QTC_LIB_DEPENDS += utils ssh
QTC_PLUGIN_DEPENDS += debugger projectexplorer
include(../../../../qtcreator.pri)
include(../../qttestrpath.pri)
include($$IDE_SOURCE_TREE/src/plugins/valgrind/valgrind_test.pri)

TEMPLATE = app
TARGET = modeldemo

macx:CONFIG -= app_bundle

QT += gui network

DEFINES += "PARSERTESTS_DATA_DIR=\\\"$$PWD/data\\\""
DEFINES += "VALGRIND_FAKE_PATH=\\\"$$IDE_BUILD_TREE/src/tools/valgrindfake/valgrind-fake\\\""

SOURCES += modeldemo.cpp

HEADERS += modeldemo.h
