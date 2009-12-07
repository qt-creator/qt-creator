TARGET = qpatch
QT = core

CONFIG += console
macx:CONFIG -= app_bundle

SOURCES += qpatch.cpp

DEFINES += QT_UIC
