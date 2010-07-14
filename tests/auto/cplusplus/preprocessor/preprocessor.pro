TEMPLATE = app
CONFIG += qt warn_on console depend_includepath
CONFIG += qtestlib testcase
TARGET = tst_$$TARGET
include(../shared/shared.pri)
SOURCES += tst_preprocessor.cpp
