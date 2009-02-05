TEMPLATE = app
CONFIG += qt warn_on console depend_includepath
QT = core testlib
include(../shared/shared.pri)
SOURCES += tst_ast.cpp
