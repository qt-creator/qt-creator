TEMPLATE = app
CONFIG += qt warn_on console depend_includepath
QT += testlib
TARGET = tst_$$TARGET
include(../shared/shared.pri)
SOURCES += tst_preprocessor.cpp
