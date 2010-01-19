TEMPLATE = app

CONFIG += qt warn_on console depend_includepath
CONFIG -= app_bundle

QT += testlib \
    script \
    declarative

PLUGIN_DIR=../../../../../src/plugins/qmlprojectmanager

include($$PLUGIN_DIR/fileformat/fileformat.pri)

INCLUDEPATH += $$PLUGIN_DIR/fileformat

TARGET=tst_$$TARGET

DEFINES += SRCDIR=\\\"$$PWD\\\"

TEMPLATE = app
SOURCES += tst_fileformat.cpp
