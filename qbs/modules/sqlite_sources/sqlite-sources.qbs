Module {
    property bool buildSharedLib: true

    readonly property string libsDir: path + "/../../../src/libs"
    readonly property string sqliteDir3rdParty: libsDir + "/3rdparty/sqlite"
    readonly property string sqliteDir: libsDir + "/sqlite"
    readonly property string includeDir: sqliteDir

    Depends { name: "cpp" }

    cpp.defines: {
        var defines = ["SQLITE_CUSTOM_INCLUDE=config.h", "SQLITE_CORE"];
        if (buildSharedLib)
            defines.push("SQLITE_LIBRARY");
        else
            defines.push("SQLITE_STATIC_LIBRARY");
        if (qbs.targetOS.contains("linux"))
            defines.push("_POSIX_C_SOURCE=200809L", "_GNU_SOURCE");
        else if (qbs.targetOS.contains("macos"))
            defines.push("_BSD_SOURCE");
        return defines;
    }

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
            "carray.c",
            "config.h",
            "sqlite3.c",
            "sqlite3.h",
            "sqlite.h",
            "sqlite3ext.h",
        ]
    }
}
