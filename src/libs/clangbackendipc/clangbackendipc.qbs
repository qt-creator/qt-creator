import qbs 1.0

QtcLibrary {
    name: "ClangBackEndIpc"
    targetName: "Clangbackendipc"

    Depends { name: "Qt.network" }
    Depends { name: "Sqlite" }
    Depends { name: "Utils" }

    cpp.defines: base.concat("CLANGBACKENDIPC_BUILD_LIB")
    cpp.includePaths: base.concat(".")

    Group {
        files: [
            "*.h",
            "*.cpp"
        ]
    }

    Export {
        Depends { name: "Sqlite" }
        Depends { name: "Utils" }
        Depends { name: "Qt.network" }
        cpp.includePaths: [
            "."
        ]
    }
}
