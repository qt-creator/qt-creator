import qbs 1.0

QtcLibrary {
    name: "Sqlite"

    cpp.includePaths: base.concat(["../3rdparty/sqlite", "."])
    cpp.defines: base.concat([
        "BUILD_SQLITE_LIBRARY",
        "SQLITE_THREADSAFE=2",
        "SQLITE_ENABLE_FTS4",
        "SQLITE_ENABLE_FTS3_PARENTHESIS",
        "SQLITE_ENABLE_UNLOCK_NOTIFY",
        "SQLITE_ENABLE_COLUMN_METADATA"
    ])
    cpp.optimization: "fast"
    cpp.dynamicLibraries: base.concat((qbs.targetOS.contains("unix") && !qbs.targetOS.contains("bsd"))
                                      ? ["dl", "pthread"] : [])


    Group {
        name: "ThirdPartySqlite"
        prefix: "../3rdparty/sqlite/"
        cpp.warningLevel: "none"
        files: [
            "sqlite3.c",
            "sqlite3.h",
            "sqlite3ext.h",
        ]
    }

    Group {
        files: [
            "*.h",
            "*.cpp"
        ]
    }

    Export {
        cpp.includePaths: base.concat([
            "../3rdparty/sqlite",
            "."
        ])
    }
}
