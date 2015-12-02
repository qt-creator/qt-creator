unix:QMAKE_CXXFLAGS_DEBUG += -O2
win32:QMAKE_CXXFLAGS_DEBUG += -O2

include(../../qtcreatorlibrary.pri)

win32:DEFINES += SQLITE_API=__declspec(dllexport)
unix:DEFINES += SQLITE_API=\"__attribute__((visibility(\\\"default\\\")))\"

include(sqlite-lib.pri)
