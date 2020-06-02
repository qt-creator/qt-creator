shared {
    DEFINES += BUILD_SQLITE_LIBRARY
} else {
    DEFINES += BUILD_SQLITE_STATIC_LIBRARY
}

INCLUDEPATH += $$PWD

unix:!bsd: LIBS += -ldl

include(../3rdparty/sqlite/sqlite.pri)

SOURCES += \
    $$PWD/createtablesqlstatementbuilder.cpp \
    $$PWD/sqlitedatabasebackend.cpp \
    $$PWD/sqliteexception.cpp \
    $$PWD/sqliteglobal.cpp \
    $$PWD/sqlitereadstatement.cpp \
    $$PWD/sqlitereadwritestatement.cpp \
    $$PWD/sqlitesessionchangeset.cpp \
    $$PWD/sqlitesessions.cpp \
    $$PWD/sqlitewritestatement.cpp \
    $$PWD/sqlstatementbuilder.cpp \
    $$PWD/utf8string.cpp \
    $$PWD/utf8stringvector.cpp \
    $$PWD/sqlitedatabase.cpp \
    $$PWD/sqlitebasestatement.cpp
HEADERS += \
    $$PWD/constraints.h \
    $$PWD/tableconstraints.h \
    $$PWD/createtablesqlstatementbuilder.h \
    $$PWD/lastchangedrowid.h \
    $$PWD/sqlitedatabasebackend.h \
    $$PWD/sqlitedatabaseinterface.h \
    $$PWD/sqliteexception.h \
    $$PWD/sqliteglobal.h \
    $$PWD/sqlitereadstatement.h \
    $$PWD/sqlitereadwritestatement.h \
    $$PWD/sqlitesessionchangeset.h \
    $$PWD/sqlitesessions.h \
    $$PWD/sqlitetransaction.h \
    $$PWD/sqlitevalue.h \
    $$PWD/sqlitewritestatement.h \
    $$PWD/sqlstatementbuilder.h \
    $$PWD/sqlstatementbuilderexception.h \
    $$PWD/utf8string.h \
    $$PWD/utf8stringvector.h \
    $$PWD/sqlitedatabase.h \
    $$PWD/sqlitetable.h \
    $$PWD/sqlitecolumn.h \
    $$PWD/sqliteindex.h \
    $$PWD/sqlitebasestatement.h

DEFINES += SQLITE_THREADSAFE=2 SQLITE_ENABLE_FTS5 SQLITE_ENABLE_UNLOCK_NOTIFY \
    SQLITE_ENABLE_JSON1 SQLITE_DEFAULT_FOREIGN_KEYS=1 SQLITE_TEMP_STORE=2 \
    SQLITE_DEFAULT_WAL_SYNCHRONOUS=1 SQLITE_MAX_WORKER_THREADS SQLITE_DEFAULT_MEMSTATUS=0 \
    SQLITE_OMIT_DEPRECATED SQLITE_OMIT_DECLTYPE \
    SQLITE_MAX_EXPR_DEPTH=0 SQLITE_OMIT_SHARED_CACHE SQLITE_USE_ALLOCA  \
    SQLITE_ENABLE_MEMORY_MANAGEMENT SQLITE_ENABLE_NULL_TRIM SQLITE_OMIT_EXPLAIN \
    SQLITE_OMIT_LOAD_EXTENSION SQLITE_OMIT_UTF16 SQLITE_DQS=0 \
    SQLITE_ENABLE_STAT4 HAVE_ISNAN HAVE_FDATASYNC HAVE_MALLOC_USABLE_SIZE \
    SQLITE_DEFAULT_MMAP_SIZE=268435456 SQLITE_CORE SQLITE_ENABLE_SESSION SQLITE_ENABLE_PREUPDATE_HOOK \
    SQLITE_LIKE_DOESNT_MATCH_BLOBS

CONFIG(debug, debug|release): DEFINES += SQLITE_ENABLE_API_ARMOR

OTHER_FILES += README.md

contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols
