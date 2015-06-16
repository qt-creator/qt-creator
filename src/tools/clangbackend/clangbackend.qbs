import qbs 1.0
import QtcClangInstallation as Clang
import QtcProcessOutputReader

QtcTool {
    name: "clangbackend"

    Depends { name: "ClangBackEndIpc" }

    Group {
        prefix: "ipcsource/"
        files: [
            "*.h",
            "*.cpp"
        ]
    }

    files: [ "clangbackendmain.cpp" ]

    property string llvmConfig: Clang.llvmConfig(qbs)
    property string llvmIncludeDir: Clang.includeDir(llvmConfig, QtcProcessOutputReader)
    property string llvmLibDir: Clang.libDir(llvmConfig, QtcProcessOutputReader)
    property string llvmLibs: Clang.libraries(qbs.targetOS)

    condition: llvmConfig

    cpp.includePaths: base.concat(["ipcsource", llvmIncludeDir])
    cpp.libraryPaths: base.concat(llvmLibDir)
    cpp.dynamicLibraries: base.concat(llvmLibs)
}
