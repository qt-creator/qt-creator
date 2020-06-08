INCLUDEPATH *= $$PWD

HEADERS += $$PWD/sqlite3.h \
           $$PWD/sqlite3ext.h

SOURCES += $$PWD/sqlite3.c \
    $$PWD/carray.c

gcc {
QMAKE_CFLAGS_WARN_ON = -w
}
