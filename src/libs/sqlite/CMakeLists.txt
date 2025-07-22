env_with_default("QTC_ENABLE_SQLITE_TRACING" ENV_QTC_ENABLE_SQLITE_TRACING OFF)
option(ENABLE_SQLITE_TRACING "Enable sqlite tarcing" ${ENV_QTC_ENABLE_SQLITE_TRACING})
add_feature_info("Sqlite tracing" ${ENABLE_SQLITE_TRACING} "")

add_qtc_library(SqliteInternal OBJECT
    PROPERTIES AUTOMOC OFF AUTOUIC OFF QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON
    DEFINES SQLITE_CORE SQLITE_CUSTOM_INCLUDE=config.h $<$<CONFIG:Debug>:SQLITE_DEBUG>
    INCLUDES
      ../3rdparty/sqlite
    SOURCES
      ../3rdparty/sqlite
      ../3rdparty/sqlite/sqlite3.c
      ../3rdparty/sqlite/sqlite3.h
      ../3rdparty/sqlite/sqlite3ext.h
      ../3rdparty/sqlite/carray.c
      ../3rdparty/sqlite/config.h
      ../3rdparty/sqlite/sqlite_static_config.h
      ../3rdparty/sqlite/sqlite.h
  SBOM_ARGS
    USE_ATTRIBUTION_FILES
    CREATE_SBOM_FOR_EACH_ATTRIBUTION
    ATTRIBUTION_FILE_PATHS
      "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS
      sqlite
)

extend_qtc_library(SqliteInternal
  CONDITION QTC_STATIC_BUILD
  PRIVATE_COMPILE_OPTIONS $<IF:$<CXX_COMPILER_ID:MSVC>,/FIsqlite_static_config.h,-includesqlite_static_config.h>
)

if (APPLE)
  extend_qtc_library(SqliteInternal DEFINES _BSD_SOURCE)
elseif (UNIX)
  extend_qtc_library(SqliteInternal DEFINES _POSIX_C_SOURCE=200809L _GNU_SOURCE _DEFAULT_SOURCE)
endif()

add_qtc_library(Sqlite
  PROPERTIES AUTOMOC OFF AUTOUIC OFF
  DEPENDS Qt::Core Threads::Threads ${CMAKE_DL_LIBS} SqliteInternal Nanotrace
  INCLUDES
    ../3rdparty/sqlite
  PUBLIC_INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}"
  DEFINES SQLITE_CUSTOM_INCLUDE=config.h $<$<CONFIG:Debug>:SQLITE_REVERSE>
  SOURCES
    constraints.h
    createtablesqlstatementbuilder.h
    lastchangedrowid.h
    sourcelocation.h
    sqlite3_fwd.h
    sqlitealgorithms.h
    sqlitebasestatement.cpp sqlitebasestatement.h
    sqlitecolumn.h
    sqlitedatabase.cpp sqlitedatabase.h
    sqlitedatabasebackend.cpp sqlitedatabasebackend.h
    sqlitedatabaseinterface.h
    sqliteexception.cpp sqliteexception.h
    sqliteglobal.cpp sqliteglobal.h
    sqlitefunctionregistry.cpp sqlitefunctionregistry.h
    sqliteindex.h
    sqliteprogresshandler.h
    sqlitereadstatement.h
    sqlitereadwritestatement.h
    sqlitesessionchangeset.cpp sqlitesessionchangeset.h
    sqlitesessions.cpp sqlitesessions.h
    sqlitetable.h
    sqlitetracing.cpp sqlitetracing.h
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

extend_qtc_library(Sqlite
  CONDITION TARGET Nanotrace
  DEPENDS Nanotrace
  PUBLIC_DEFINES
    $<$<BOOL:${ENABLE_SQLITE_TRACING}>:ENABLE_SQLITE_TRACING>
)

extend_qtc_library(Sqlite
  CONDITION QTC_STATIC_BUILD
  PRIVATE_COMPILE_OPTIONS $<IF:$<CXX_COMPILER_ID:MSVC>,/FIsqlite_static_config.h,-includesqlite_static_config.h>
  PROPERTIES
    INTERFACE_COMPILE_OPTIONS $<IF:$<CXX_COMPILER_ID:MSVC>,/FI../3rdparty/sqlite/sqlite_static_config.h,-include../3rdparty/sqlite/sqlite_static_config.h>
)
