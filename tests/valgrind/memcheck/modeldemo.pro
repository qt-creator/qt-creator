include(../../../qtcreator.pri)
include(../../auto/qttestrpath.pri)
include($$IDE_SOURCE_TREE/src/libs/3rdparty/botan/botan.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)
include($$IDE_SOURCE_TREE/src/libs/ssh/ssh.pri)
include($$IDE_SOURCE_TREE/src/plugins/valgrind/valgrind_test.pri)

TEMPLATE = app
TARGET = modeldemo

macx:CONFIG -= app_bundle

QT += gui network

DEFINES += "PARSERTESTS_DATA_DIR=\\\"$$PWD/data\\\""


SOURCES += modeldemo.cpp

HEADERS += modeldemo.h
