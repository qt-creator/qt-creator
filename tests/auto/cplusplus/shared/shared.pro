
TEMPLATE = lib
TARGET = CPlusPlusTestSupport
CONFIG += static
QT = core
DESTDIR = $$PWD

DEFINES += CPLUSPLUS_WITH_NAMESPACE
include($$PWD/../../../../src/shared/cplusplus/cplusplus.pri)
