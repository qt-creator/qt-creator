include(../../../../../src/plugins/qmldesigner/config.pri)
QT += testlib

DESTDIR = $$DESIGNER_BINARY_DIRECTORY
include(../../../../../src/plugins/qmldesigner/core/core.pri)
include(../../../../../src/libs/qmljs/qmljs-lib.pri)
HEADERS+=../../../../../src/libs/utils/changeset.h
SOURCES+=../../../../../src/libs/utils/changeset.cpp
INCLUDEPATH+=../../../../../src/libs
DEFINES+=QTCREATOR_UTILS_STATIC_LIB QML_BUILD_STATIC_LIB

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
