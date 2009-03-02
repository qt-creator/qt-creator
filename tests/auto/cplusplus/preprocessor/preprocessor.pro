TEMPLATE = app
CONFIG += qt warn_on console depend_includepath
QT = core testlib
TARGET = tst_$$TARGET
DEFINES += CPLUSPLUS_WITH_NAMESPACE

include(../../../../src/libs/cplusplus/cplusplus-lib.pri)

SOURCES += tst_preprocessor.cpp
