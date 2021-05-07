Module {
    property bool buildSharedLib: true

    readonly property string libsDir: path + "/../../../src/libs"
    readonly property string sqliteDir3rdParty: libsDir + "/3rdparty/sqlite"
    readonly property string sqliteDir: libsDir + "/sqlite"
    readonly property string includeDir: sqliteDir

    Depends { name: "cpp" }

    cpp.defines: [
        "_HAVE_SQLITE_CONFIG_H", "SQLITE_CORE"
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
