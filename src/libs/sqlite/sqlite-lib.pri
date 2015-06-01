contains(CONFIG, dll) {
    DEFINES += BUILD_SQLITE_LIBRARY
} else {
    DEFINES += BUILD_SQLITE_STATIC_LIBRARY
}

INCLUDEPATH += $$PWD

unix:LIBS += -ldl

include(../3rdparty/sqlite/sqlite.pri)

SOURCES += \
    $$PWD/columndefinition.cpp \
    $$PWD/createtablecommand.cpp \
    $$PWD/createtablesqlstatementbuilder.cpp \
    $$PWD/sqlitedatabasebackend.cpp \
    $$PWD/sqlitedatabaseconnection.cpp \
    $$PWD/sqlitedatabaseconnectionproxy.cpp \
    $$PWD/sqliteexception.cpp \
    $$PWD/sqliteglobal.cpp \
    $$PWD/sqlitereadstatement.cpp \
    $$PWD/sqlitereadwritestatement.cpp \
    $$PWD/sqlitestatement.cpp \
    $$PWD/sqlitetransaction.cpp \
    $$PWD/sqliteworkerthread.cpp \
    $$PWD/sqlitewritestatement.cpp \
    $$PWD/sqlstatementbuilder.cpp \
    $$PWD/sqlstatementbuilderexception.cpp \
    $$PWD/utf8string.cpp \
    $$PWD/utf8stringvector.cpp \
    $$PWD/sqlitedatabase.cpp \
    $$PWD/sqlitetable.cpp \
    $$PWD/sqlitecolumn.cpp \
    $$PWD/tablewriteworker.cpp \
    $$PWD/tablewriteworkerproxy.cpp
HEADERS += \
    $$PWD/columndefinition.h \
    $$PWD/createtablesqlstatementbuilder.h \
    $$PWD/sqlitedatabasebackend.h \
    $$PWD/sqlitedatabaseconnection.h \
    $$PWD/sqlitedatabaseconnectionproxy.h \
    $$PWD/sqliteexception.h \
    $$PWD/sqliteglobal.h \
    $$PWD/sqlitereadstatement.h \
    $$PWD/sqlitereadwritestatement.h \
    $$PWD/sqlitestatement.h \
    $$PWD/sqlitetransaction.h \
    $$PWD/sqliteworkerthread.h \
    $$PWD/sqlitewritestatement.h \
    $$PWD/sqlstatementbuilder.h \
    $$PWD/sqlstatementbuilderexception.h \
    $$PWD/utf8string.h \
    $$PWD/utf8stringvector.h \
    $$PWD/sqlitedatabase.h \
    $$PWD/sqlitetable.h \
    $$PWD/sqlitecolumn.h \
    $$PWD/tablewriteworker.h \
    $$PWD/tablewriteworkerproxy.h \
    $$PWD/createtablecommand.h

DEFINES += SQLITE_THREADSAFE=2 SQLITE_ENABLE_FTS4 SQLITE_ENABLE_FTS3_PARENTHESIS SQLITE_ENABLE_UNLOCK_NOTIFY SQLITE_ENABLE_COLUMN_METADATA

contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols
