add_qtc_library(Sqlite
  DEFINES
    SQLITE_THREADSAFE=2 SQLITE_ENABLE_FTS4 SQLITE_ENABLE_FTS3_PARENTHESIS
    SQLITE_ENABLE_UNLOCK_NOTIFY SQLITE_ENABLE_COLUMN_METADATA
    BUILD_SQLITE_LIBRARY
  DEPENDS Qt5::Core Threads::Threads ${CMAKE_DL_LIBS}
  PUBLIC_INCLUDES ../3rdparty/sqlite
  SOURCES
    ../3rdparty/sqlite/sqlite3.c
    createtablesqlstatementbuilder.cpp createtablesqlstatementbuilder.h
    sqlitebasestatement.cpp sqlitebasestatement.h
    sqlitecolumn.h
    sqlitedatabase.cpp sqlitedatabase.h
    sqlitedatabasebackend.cpp sqlitedatabasebackend.h
    sqliteexception.cpp sqliteexception.h
    sqliteglobal.cpp sqliteglobal.h
    sqliteindex.h
    sqlitereadstatement.cpp sqlitereadstatement.h
    sqlitereadwritestatement.cpp sqlitereadwritestatement.h
    sqlitetable.h
    sqlitetransaction.h
    sqlitewritestatement.cpp sqlitewritestatement.h
    sqlstatementbuilder.cpp sqlstatementbuilder.h
    sqlstatementbuilderexception.h
    utf8string.cpp utf8string.h
    utf8stringvector.cpp utf8stringvector.h
)
