TEMPLATE = lib
TARGET = CPlusPlus

#DEFINES += NDEBUG
#unix:QMAKE_CXXFLAGS_DEBUG += -O2
QMAKE_CXXFLAGS_DEBUG += -ggdb
include(../../qtcreatorlibrary.pri)
include(cplusplus-lib.pri)
include(../languageutils/languageutils.pri)
