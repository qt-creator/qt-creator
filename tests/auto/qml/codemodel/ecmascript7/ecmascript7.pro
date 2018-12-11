QTC_LIB_DEPENDS += qmljs
QTC_PLUGIN_DEPENDS += qmljstools

include(../../../qttest.pri)

DEFINES+=QTCREATORDIR=\\\"$$IDE_SOURCE_TREE\\\"
DEFINES+=TESTSRCDIR=\\\"$$PWD\\\"

QT += core
QT -= gui

TARGET = tst_ecmascript7
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += tst_ecmascript7.cpp

DISTFILES += \
    samples/basic/arrow-functions.js \
    samples/basic/class.js \
    samples/basic/constructor.js \
    samples/basic/extends.js \
    samples/basic/let.js \
    samples/basic/super.js \
    samples/basic/template-strings.js \
    samples/basic/yield.js
