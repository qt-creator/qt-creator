import qbs
import qbs.FileInfo

QtcTool {
    name: "clangrefactoringbackend"
    Depends { name: "libclang"; required: false }
    condition: libclang.present
               && libclang.toolingEnabled
               && (!qbs.targetOS.contains("windows") || libclang.llvmBuildModeMatches)

    Depends { name: "ClangSupport" }

    Depends { name: "Qt.network" }

    cpp.cxxFlags: base.concat(libclang.llvmToolingCxxFlags)
    cpp.defines: base.concat(libclang.llvmToolingDefines)
    cpp.includePaths: base.concat(libclang.llvmIncludeDir)
                          .concat(libclang.llvmToolingIncludes)
                          .concat(["source"])
                          .concat(["../clangpchmanagerbackend/source"])
    cpp.libraryPaths: base.concat(libclang.llvmLibDir)
    cpp.dynamicLibraries: base.concat(libclang.llvmToolingLibs)

    Properties {
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("macos")
        cpp.rpaths: base.concat(libclang.llvmLibDir)
    }

    files: [
        "clangrefactoringbackendmain.cpp",
    ]

    Group {
        prefix: "source/"
        files: [
            "*.cpp",
            "*.h",
        ]
    }
}
