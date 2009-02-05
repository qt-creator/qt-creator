
TEMPLATE = lib
TARGET = CPlusPlusTestSupport
CONFIG += static
QT = core

DEFINES += HAVE_QT CPLUSPLUS_WITH_NAMESPACE
include($$PWD/../../../../src/shared/cplusplus/cplusplus.pri)
