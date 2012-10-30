include(../../../../../qtcreator.pri)
#include(../../../../../src/plugins/qmldesigner/config.pri)
QT += testlib
CONFIG += testcase

##DEFINES += DONT_MESS_WITH_QDEBUG

INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins/qmldesigner/designercore/include

TARGET = tst_bauhaus
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += testbauhaus.cpp
HEADERS += testbauhaus.h
DEFINES += WORKDIR=\\\"$$DESTDIR\\\"
