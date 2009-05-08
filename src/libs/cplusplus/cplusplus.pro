TEMPLATE = lib

TARGET = CPlusPlus

DEFINES += NDEBUG
unix:QMAKE_CXXFLAGS_DEBUG += -O3

include(../../qtcreatorlibrary.pri)
include(cplusplus-lib.pri)
