TARGET = qpatch
QT = core
CONFIG += console
macx:CONFIG -= app_bundle
SOURCES += qpatch.cpp

QT_BUILD_TREE=$$fromfile($$(QTDIR)/.qmake.cache,QT_BUILD_TREE)
QT_SOURCE_TREE=$$fromfile($$(QTDIR)/.qmake.cache,QT_SOURCE_TREE)

DEFINES += QT_UIC
include(bootstrap.pri)

message($$QT_BUILD_TREE)

