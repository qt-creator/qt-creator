include(../../../../../src/plugins/qmldesigner/config.pri)
QT += testlib

DESTDIR = $$DESIGNER_BINARY_DIRECTORY
include(../../../../../src/plugins/qmldesigner/core/core.pri)

##DEFINES += DONT_MESS_WITH_QDEBUG

DEPENDPATH += ..
DEPENDPATH += ../../../../../src/plugins/qmldesigner/core/include

TARGET = tst_bauhaus
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += testbauhaus.cpp
HEADERS += testbauhaus.h
DEFINES += WORKDIR=\\\"$$DESTDIR\\\"
