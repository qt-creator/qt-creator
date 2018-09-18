import qbs
import qbs.FileInfo

QtcPlugin {
    name: "ClangFormat"

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "Utils" }

    Depends { name: "libclang"; required: false }
    Depends { name: "clang_defines" }

    Depends { name: "Qt.widgets" }

    condition: libclang.present
               && (!qbs.targetOS.contains("windows") || libclang.llvmBuildModeMatches)

    cpp.defines: base.concat("CLANGPCHMANAGER_LIB")
    cpp.includePaths: base.concat(libclang.llvmIncludeDir)
    cpp.libraryPaths: base.concat(libclang.llvmLibDir)
    cpp.dynamicLibraries: base.concat(libclang.llvmFormattingLibs)
    cpp.rpaths: base.concat(libclang.llvmLibDir)

    files: [
        "clangformatconfigwidget.cpp",
        "clangformatconfigwidget.h",
        "clangformatconfigwidget.ui",
        "clangformatindenter.cpp",
        "clangformatindenter.h",
        "clangformatplugin.cpp",
        "clangformatplugin.h",
        "clangformatconstants.h",
    ]
}
