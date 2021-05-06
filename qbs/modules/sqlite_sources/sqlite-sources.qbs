Module {
    property bool buildSharedLib: true

    readonly property string libsDir: path + "/../../../src/libs"
    readonly property string sqliteDir3rdParty: libsDir + "/3rdparty/sqlite"
    readonly property string sqliteDir: libsDir + "/sqlite"
    readonly property string includeDir: sqliteDir

    Depends { name: "cpp" }

    cpp.defines: [
        "SQLITE_THREADSAFE=2", "SQLITE_ENABLE_FTS5", "SQLITE_ENABLE_UNLOCK_NOTIFY",
        "SQLITE_ENABLE_JSON1", "SQLITE_DEFAULT_FOREIGN_KEYS=1", "SQLITE_TEMP_STORE=2",
        "SQLITE_DEFAULT_WAL_SYNCHRONOUS=1", "SQLITE_MAX_WORKER_THREADS", "SQLITE_DEFAULT_MEMSTATUS=0",
        "SQLITE_OMIT_DEPRECATED", "SQLITE_OMIT_DECLTYPE",
        "SQLITE_MAX_EXPR_DEPTH=0", "SQLITE_OMIT_SHARED_CACHE", "SQLITE_USE_ALLOCA",
        "SQLITE_ENABLE_MEMORY_MANAGEMENT", "SQLITE_ENABLE_NULL_TRIM", "SQLITE_OMIT_EXPLAIN",
        "SQLITE_OMIT_LOAD_EXTENSION", "SQLITE_OMIT_UTF16", "SQLITE_DQS=0",
        "SQLITE_ENABLE_STAT4", "HAVE_ISNAN", "HAVE_FDATASYNC", "HAVE_MALLOC_USABLE_SIZE",
        "SQLITE_DEFAULT_MMAP_SIZE=268435456", "SQLITE_CORE", "SQLITE_ENABLE_SESSION", "SQLITE_ENABLE_PREUPDATE_HOOK",
        "SQLITE_LIKE_DOESNT_MATCH_BLOBS",
    ].concat(buildSharedLib ? "BUILD_SQLITE_LIBRARY" : "BUILD_SQLITE_STATIC_LIBRARY")

    cpp.dynamicLibraries: base.concat((qbs.targetOS.contains("unix") && !qbs.targetOS.contains("bsd"))
                                      ? ["dl", "pthread"] : [])
    cpp.includePaths: base.concat([sqliteDir3rdParty, sqliteDir])
    cpp.optimization: "fast"

    Group {
        name: "wrapper sources"
        prefix: sqlite_sources.sqliteDir + '/'
        files: [
            "*.h",
            "*.cpp"
        ]
    }

    Group {
        name: "sqlite sources"
        prefix: sqlite_sources.sqliteDir3rdParty + '/'
        cpp.warningLevel: "none"
        files: [
            "sqlite3.c",
            "sqlite3.h",
            "sqlite3ext.h",
            "carray.c"
        ]
    }
}
