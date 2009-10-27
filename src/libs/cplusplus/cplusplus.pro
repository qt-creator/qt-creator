TEMPLATE = lib
CONFIG+=dll
TARGET = CPlusPlus

DEFINES += NDEBUG
unix:QMAKE_CXXFLAGS_DEBUG += -O3

include(../../qtcreatorlibrary.pri)
include(cplusplus-lib.pri)
