TEMPLATE = lib

TARGET = CPlusPlus

DEFINES += NDEBUG
unix:QMAKE_CXXFLAGS_DEBUG += -O3

include(../../qworkbenchlibrary.pri)
include(cplusplus-lib.pri)
