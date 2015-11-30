INCLUDEPATH *= $$PWD

HEADERS += $$PWD/okapi_bm25.h \
           $$PWD/sqlite3.h \
           $$PWD/sqlite3ext.h


SOURCES += $$PWD/sqlite3.c

win32:DEFINES += SQLITE_API=__declspec(dllexport)
unix:DEFINES += SQLITE_API=\"__attribute__((visibility(\\\"default\\\")))\"

gcc {
QMAKE_CFLAGS_WARN_ON = -w
}
