import qbs

QtcPlugin {
    name: "ClangFormat"

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "CppEditor" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Utils" }

    Depends { name: "libclang"; required: false }
    Depends { name: "clang_defines" }

    Depends { name: "Qt.widgets" }

    condition: libclang.present
               && libclang.llvmFormattingLibs.length
               && (!qbs.targetOS.contains("windows") || libclang.llvmBuildModeMatches)

    cpp.cxxFlags: {
        var res = base.concat(libclang.llvmToolingCxxFlags);
        if (qbs.toolchain.contains("gcc"))
            res.push("-Wno-comment"); // clang/Format/Format.h has intentional multiline comments
        return res;
    }
    cpp.defines: base.concat("CLANGPCHMANAGER_LIB")
    cpp.includePaths: base.concat(libclang.llvmIncludeDir)
    cpp.libraryPaths: base.concat(libclang.llvmLibDir)
    cpp.dynamicLibraries: base.concat(libclang.llvmFormattingLibs)
    cpp.rpaths: base.concat(libclang.llvmLibDir)

    files: [
        "clangformatbaseindenter.h",
        "clangformatbaseindenter.cpp",
        "clangformatconfigwidget.cpp",
        "clangformatconfigwidget.h",
        "clangformatconfigwidget.ui",
        "clangformatconstants.h",
        "clangformatindenter.cpp",
        "clangformatindenter.h",
        "clangformatplugin.cpp",
        "clangformatplugin.h",
        "clangformatsettings.cpp",
        "clangformatsettings.h",
        "clangformatutils.h",
        "clangformatutils.cpp",
    ]
}
