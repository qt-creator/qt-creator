TEMPLATE = app
CONFIG += qt warn_on console depend_includepath
QT = core testlib

include(../../../../src/libs/cplusplus/cplusplus-lib.pri)

SOURCES += tst_lookup.cpp
