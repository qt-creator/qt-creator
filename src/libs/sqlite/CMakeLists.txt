set(SQLITE_SOURCES
  ../3rdparty/sqlite/sqlite3.c
  ../3rdparty/sqlite/sqlite3.h
  ../3rdparty/sqlite/carray.c
  ../3rdparty/sqlite/config.h
  ../3rdparty/sqlite/sqlite.h
)

add_qtc_library(Sqlite
  PROPERTIES AUTOMOC OFF AUTOUIC OFF
  PUBLIC_DEFINES
      SQLITE_CORE
  DEPENDS Qt5::Core Threads::Threads ${CMAKE_DL_LIBS}
  PUBLIC_INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}"
    ../3rdparty/sqlite
  SOURCES
    ${SQLITE_SOURCES}
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
)

set_source_files_properties(
  ${SQLITE_SOURCES}
  PROPERTIES
    QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON
)

extend_qtc_library(Sqlite DEFINES SQLITE_CUSTOM_INCLUDE=config.h)

if (APPLE)
  extend_qtc_library(Sqlite DEFINES _BSD_SOURCE)
elseif (UNIX)
  extend_qtc_library(Sqlite DEFINES _POSIX_C_SOURCE=200809L _GNU_SOURCE _DEFAULT_SOURCE)
endif()
