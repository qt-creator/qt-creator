QTC_LIB_DEPENDS += qmljs
QTC_PLUGIN_DEPENDS += qmljstools

include(../../../qttest.pri)

DEFINES+=QTCREATORDIR=\\\"$$IDE_SOURCE_TREE\\\"
DEFINES+=TESTSRCDIR=\\\"$$PWD\\\"

QT += core
QT -= gui

TARGET = tst_dependencies
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += tst_dependencies.cpp
