TEMPLATE = app
CONFIG += qt warn_on console depend_includepath
CONFIG += qtestlib testcase
CONFIG -= app_bundle
include(../shared/shared.pri)
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins
SOURCES += tst_codegen.cpp

