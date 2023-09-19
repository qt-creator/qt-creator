import qbs.Utilities

QtcLibrary {
    name: "Sqlite"

    Depends { name: "Utils" }
    Depends { name: "sqlite_sources" }
    Depends { name: "Qt.core"; required:false }
    condition: Qt.core.present && Utilities.versionCompare(Qt.core.version, "6.4.3") >= 0

    property string exportedIncludeDir: sqlite_sources.includeDir

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: exportingProduct.exportedIncludeDir
    }
}
