DEFINES += NDEBUG
unix:QMAKE_CXXFLAGS_DEBUG += -O2
win32:QMAKE_CXXFLAGS_DEBUG += -O2

include(../../qtcreatorlibrary.pri)
include(cplusplus-lib.pri)
