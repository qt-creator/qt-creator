import qbs 1.0

QtcLibrary {
    name: "Sqlite"

    Depends { name: "sqlite_sources" }
    property string exportedIncludeDir: sqlite_sources.includeDir

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: product.exportedIncludeDir
    }
}
