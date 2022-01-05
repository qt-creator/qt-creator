shared {
    DEFINES += BUILD_SQLITE_LIBRARY
} else {
    DEFINES += BUILD_SQLITE_STATIC_LIBRARY
}

INCLUDEPATH += $$PWD

unix:!bsd: LIBS += -ldl

include(../3rdparty/sqlite/sqlite.pri)

SOURCES += \
    $$PWD/sqlitedatabasebackend.cpp \
    $$PWD/sqliteexception.cpp \
    $$PWD/sqliteglobal.cpp \
    $$PWD/sqlitelibraryinitializer.cpp \
    $$PWD/sqlitesessionchangeset.cpp \
    $$PWD/sqlitesessions.cpp \
    $$PWD/sqlstatementbuilder.cpp \
    $$PWD/utf8string.cpp \
    $$PWD/utf8stringvector.cpp \
    $$PWD/sqlitedatabase.cpp \
    $$PWD/sqlitebasestatement.cpp
HEADERS += \
    $$PWD/constraints.h \
    $$PWD/sqlitealgorithms.h \
    $$PWD/sqliteblob.h \
    $$PWD/sqlitelibraryinitializer.h \
    $$PWD/sqlitetimestamp.h \
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

DEFINES += SQLITE_CUSTOM_INCLUDE=config.h SQLITE_CORE

CONFIG(debug, debug|release): DEFINES += SQLITE_ENABLE_API_ARMOR

OTHER_FILES += README.md

contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols

CONFIG += exceptions
