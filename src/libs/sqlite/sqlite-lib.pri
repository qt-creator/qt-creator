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
    $$PWD/sqlitewritestatement.cpp \
    $$PWD/sqlstatementbuilder.cpp \
    $$PWD/sqlstatementbuilderexception.cpp \
    $$PWD/utf8string.cpp \
    $$PWD/utf8stringvector.cpp \
    $$PWD/sqlitedatabase.cpp \
    $$PWD/sqlitetable.cpp \
    $$PWD/sqlitecolumn.cpp \
    $$PWD/sqlitebasestatement.cpp
HEADERS += \
    $$PWD/createtablesqlstatementbuilder.h \
    $$PWD/sqlitedatabasebackend.h \
    $$PWD/sqliteexception.h \
    $$PWD/sqliteglobal.h \
    $$PWD/sqlitereadstatement.h \
    $$PWD/sqlitereadwritestatement.h \
    $$PWD/sqlitetransaction.h \
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

DEFINES += SQLITE_THREADSAFE=2 SQLITE_ENABLE_FTS4 SQLITE_ENABLE_FTS3_PARENTHESIS \
    SQLITE_ENABLE_UNLOCK_NOTIFY SQLITE_ENABLE_COLUMN_METADATA SQLITE_ENABLE_JSON1

contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols
