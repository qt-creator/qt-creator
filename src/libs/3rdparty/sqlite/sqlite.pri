INCLUDEPATH *= $$PWD

linux:DEFINES += _POSIX_C_SOURCE=200809L _GNU_SOURCE
osx:DEFINES += _BSD_SOURCE


HEADERS += $$PWD/sqlite3.h \
           $$PWD/config.h \
           $$PWD/sqlite.h \
           $$PWD/sqlite3ext.h

SOURCES += $$PWD/sqlite3.c \
    $$PWD/carray.c

gcc {
QMAKE_CFLAGS_WARN_ON = -w
}
