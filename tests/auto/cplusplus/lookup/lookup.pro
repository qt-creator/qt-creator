TEMPLATE = app
CONFIG += qt warn_on console depend_includepath
QT += testlib
include(../shared/shared.pri)
SOURCES += tst_lookup.cpp
TARGET=tst_$$TARGET
