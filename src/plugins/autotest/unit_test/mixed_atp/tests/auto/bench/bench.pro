QT       += testlib

QT       -= gui

TARGET = tst_benchtest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += tst_benchtest.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
