add_qtc_library(Sqlite
  PROPERTIES AUTOMOC OFF AUTOUIC OFF
  PUBLIC_DEFINES
      BUILD_SQLITE_LIBRARY
      SQLITE_CORE
  DEPENDS Qt5::Core Threads::Threads ${CMAKE_DL_LIBS}
  PUBLIC_INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}"
    ../3rdparty/sqlite
  SOURCES
    ../3rdparty/sqlite/sqlite3.c
    ../3rdparty/sqlite/sqlite3.h
    ../3rdparty/sqlite/carray.c
    ../3rdparty/sqlite/config.h
    ../3rdparty/sqlite/sqlite.h
    constraints.h
    createtablesqlstatementbuilder.cpp createtablesqlstatementbuilder.h
    lastchangedrowid.h
    sqlitealgorithms.h
    sqlitebasestatement.cpp sqlitebasestatement.h
    sqlitecolumn.h
    sqlitedatabase.cpp sqlitedatabase.h
    sqlitedatabasebackend.cpp sqlitedatabasebackend.h
    sqlitedatabaseinterface.h
    sqliteexception.cpp sqliteexception.h
    sqliteglobal.cpp sqliteglobal.h
    sqliteindex.h
    sqlitereadstatement.h
    sqlitereadwritestatement.h
    sqlitesessionchangeset.cpp sqlitesessionchangeset.h
    sqlitesessions.cpp sqlitesessions.h
    sqlitetable.h
    sqlitetransaction.h
    sqlitetransaction.h
    sqlitevalue.h
    sqlitewritestatement.h
    sqlstatementbuilder.cpp sqlstatementbuilder.h
    sqlstatementbuilderexception.h
    tableconstraints.h
    utf8string.cpp utf8string.h
    utf8stringvector.cpp utf8stringvector.h
    sqliteblob.h
    sqlitetimestamp.h
    sqlitelibraryinitializer.cpp sqlitelibraryinitializer.h
)

extend_qtc_library(Sqlite DEFINES _HAVE_SQLITE_CONFIG_H)

if (APPLE)
extend_qtc_library(Sqlite DEFINES _BSD_SOURCE)
elseif (UNIX)
extend_qtc_library(Sqlite DEFINES _POSIX_C_SOURCE=200809L _GNU_SOURCE _DEFAULT_SOURCE)
endif()
