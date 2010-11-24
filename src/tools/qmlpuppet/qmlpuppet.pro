TARGET = qmlpuppet

TEMPLATE = app

QT += core gui declarative network webkit

DEFINES += QWEAKPOINTER_ENABLE_ARROW

include(../../../qtcreator.pri)
include(../../private_headers.pri)
DESTDIR = $$IDE_BIN_PATH
include(../../rpath.pri)

include (../../plugins/qmldesigner/designercore/instances/instances.pri)
include (../../plugins/qmldesigner/designercore/exceptions/exceptions.pri)

CONFIG -= app_bundle

SOURCES += main.cpp
