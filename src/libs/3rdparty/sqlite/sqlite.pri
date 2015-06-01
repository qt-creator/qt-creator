INCLUDEPATH *= $$PWD
VPATH *= $$PWD

HEADERS += okapi_bm25.h \
           sqlite3.h \
           sqlite3ext.h


SOURCES += sqlite3.c

win32:DEFINES += SQLITE_API=__declspec(dllexport)
unix:DEFINES += SQLITE_API=\"__attribute__((visibility(\\\"default\\\")))\"
