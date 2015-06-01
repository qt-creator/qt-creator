import qbs 1.0
import QtcClangInstallation as Clang
import QtcProcessOutputReader

QtcTool {
    name: "codemodelbackend"

    Depends { name: "CodeModelBackEndIpc" }

    Group {
        prefix: "ipcsource/"
        files: [
            "*.h",
            "*.cpp"
        ]
    }

    files: [ "codemodelbackendmain.cpp" ]

    property string llvmConfig: Clang.llvmConfig(qbs)
    property string llvmIncludeDir: Clang.includeDir(llvmConfig, QtcProcessOutputReader)
    property string llvmLibDir: Clang.libDir(llvmConfig, QtcProcessOutputReader)
    property string llvmLibs: Clang.libraries(qbs.targetOS)

    condition: llvmConfig

    cpp.includePaths: base.concat(["ipcsource", llvmIncludeDir])
    cpp.libraryPaths: base.concat(llvmLibDir)
    cpp.dynamicLibraries: base.concat(llvmLibs)
}
