include(../../../qttest.pri)

QT += script \
    declarative

PLUGIN_DIR=../../../../../src/plugins/qmlprojectmanager

include($$PLUGIN_DIR/fileformat/fileformat.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)

INCLUDEPATH += $$PLUGIN_DIR/fileformat

DEFINES += SRCDIR=\\\"$$PWD\\\"

TEMPLATE = app
SOURCES += tst_fileformat.cpp
