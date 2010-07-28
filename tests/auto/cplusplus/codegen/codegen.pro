TEMPLATE = app
CONFIG += qt warn_on console depend_includepath
CONFIG += qtestlib testcase
CONFIG -= app_bundle
include(../shared/shared.pri)
SOURCES += tst_codegen.cpp
TARGET=tst_$$TARGET
