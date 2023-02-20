add_qtc_library(SqliteC OBJECT
    PROPERTIES AUTOMOC OFF AUTOUIC OFF QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON POSITION_INDEPENDENT_CODE ON
    CONDITION Qt5_VERSION VERSION_GREATER_EQUAL 6.2.0
    DEFINES SQLITE_CORE SQLITE_CUSTOM_INCLUDE=config.h $<$<CONFIG:Debug>:SQLITE_DEBUG>
    SOURCES
      ../3rdparty/sqlite
      ../3rdparty/sqlite/sqlite3.c
      ../3rdparty/sqlite/sqlite3.h
      ../3rdparty/sqlite/sqlite3ext.h
      ../3rdparty/sqlite/carray.c
      ../3rdparty/sqlite/config.h
      ../3rdparty/sqlite/sqlite.h
)

add_qtc_library(Sqlite
  PROPERTIES AUTOMOC OFF AUTOUIC OFF
  DEPENDS Qt5::Core Threads::Threads ${CMAKE_DL_LIBS} SqliteC
  CONDITION Qt5_VERSION VERSION_GREATER_EQUAL 6.2.0
  PUBLIC_INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}"
    ../3rdparty/sqlite
  DEFINES SQLITE_LIBRARY SQLITE_CUSTOM_INCLUDE=config.h $<$<CONFIG:Debug>:SQLITE_REVERSE>
  SOURCES
    constraints.h
    createtablesqlstatementbuilder.h
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
    sqliteprogresshandler.h
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
    sqliteblob.h
    sqlitetimestamp.h
    sqlitelibraryinitializer.cpp sqlitelibraryinitializer.h
    sqliteids.h
)

if (APPLE)
  extend_qtc_library(SqliteC DEFINES _BSD_SOURCE)
elseif (UNIX)
  extend_qtc_library(SqliteC DEFINES _POSIX_C_SOURCE=200809L _GNU_SOURCE _DEFAULT_SOURCE)
endif()
