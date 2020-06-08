add_qtc_library(Sqlite
  PUBLIC_DEFINES
      BUILD_SQLITE_LIBRARY
      SQLITE_THREADSAFE=2 SQLITE_ENABLE_FTS5 SQLITE_ENABLE_UNLOCK_NOTIFY
      SQLITE_ENABLE_JSON1 SQLITE_DEFAULT_FOREIGN_KEYS=1 SQLITE_TEMP_STORE=2
      SQLITE_DEFAULT_WAL_SYNCHRONOUS=1 SQLITE_MAX_WORKER_THREADS SQLITE_DEFAULT_MEMSTATUS=0
      SQLITE_OMIT_DEPRECATED SQLITE_OMIT_DECLTYPE
      SQLITE_MAX_EXPR_DEPTH=0 SQLITE_OMIT_SHARED_CACHE SQLITE_USE_ALLOCA
      SQLITE_ENABLE_MEMORY_MANAGEMENT SQLITE_ENABLE_NULL_TRIM SQLITE_OMIT_EXPLAIN
      SQLITE_OMIT_LOAD_EXTENSION SQLITE_OMIT_UTF16 SQLITE_DQS=0
      SQLITE_ENABLE_STAT4 HAVE_ISNAN HAVE_FDATASYNC HAVE_MALLOC_USABLE_SIZE
      SQLITE_DEFAULT_MMAP_SIZE=268435456 SQLITE_CORE SQLITE_ENABLE_SESSION SQLITE_ENABLE_PREUPDATE_HOOK
      SQLITE_LIKE_DOESNT_MATCH_BLOBS
  DEPENDS Qt5::Core Threads::Threads ${CMAKE_DL_LIBS}
  PUBLIC_INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}"
    ../3rdparty/sqlite
  SOURCES
    ../3rdparty/sqlite/sqlite3.c
    ../3rdparty/sqlite/carray.c
    constraints.h
    createtablesqlstatementbuilder.cpp createtablesqlstatementbuilder.h
    lastchangedrowid.h
    sqlitebasestatement.cpp sqlitebasestatement.h
    sqlitecolumn.h
    sqlitedatabase.cpp sqlitedatabase.h
    sqlitedatabasebackend.cpp sqlitedatabasebackend.h
    sqliteexception.cpp sqliteexception.h
    sqliteglobal.cpp sqliteglobal.h
    sqliteindex.h
    sqlitereadstatement.cpp sqlitereadstatement.h
    sqlitereadwritestatement.cpp sqlitereadwritestatement.h
    sqlitesessionchangeset.cpp sqlitesessionchangeset.h
    sqlitesessions.cpp sqlitesessions.h
    sqlitetable.h
    sqlitetransaction.h
    sqlitewritestatement.cpp sqlitewritestatement.h
    sqlitevalue.h
    sqlstatementbuilder.cpp sqlstatementbuilder.h
    sqlstatementbuilderexception.h
    utf8string.cpp utf8string.h
    utf8stringvector.cpp utf8stringvector.h
)
