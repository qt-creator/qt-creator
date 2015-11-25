INCLUDEPATH *= $$PWD

HEADERS += $$PWD/okapi_bm25.h \
           $$PWD/sqlite3.h \
           $$PWD/sqlite3ext.h

SOURCES += $$PWD/sqlite3.c

gcc {
QMAKE_CFLAGS_WARN_ON = -w
}
