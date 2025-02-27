import qbs.Utilities

QtcLibrary {
    name: "Sqlite"

    Depends { name: "Nanotrace" }
    Depends { name: "Utils" }
    Depends { name: "sqlite_sources" }
    Depends { name: "Qt.core"; required:false }

    condition: {
        if (qbs.toolchain.contains("msvc")
                && Utilities.versionCompare(cpp.compilerVersion, "19.30.0") < 0)
            return false;

        if (qbs.targetOS.contains("macos") && qbs.toolchain.contains("clang")
                && Utilities.versionCompare(cpp.compilerVersion, "15.0.0") < 0)
            return false;

        return Qt.core.present && Utilities.versionCompare(Qt.core.version, "6.4.3") >= 0;
    }

    property string exportedIncludeDir: sqlite_sources.includeDir

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: exportingProduct.exportedIncludeDir
    }
}
