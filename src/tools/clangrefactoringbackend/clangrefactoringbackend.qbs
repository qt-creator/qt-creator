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
    cpp.includePaths: base.concat(libclang.llvmIncludeDir).concat(libclang.llvmToolingIncludes)
                          .concat(["source"])
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
            "clangquery.cpp",
            "clangquerygatherer.cpp",
            "clangquerygatherer.h",
            "clangquery.h",
            "clangrefactoringbackend_global.h",
            "clangtool.cpp",
            "clangtool.h",
            "collectmacrossourcefilecallbacks.cpp",
            "collectmacrossourcefilecallbacks.h",
            "collectsymbolsaction.cpp",
            "collectsymbolsaction.h",
            "collectsymbolsastvisitor.h",
            "collectsymbolsconsumer.h",
            "findcursorusr.h",
            "findlocationsofusrs.h",
            "findusrforcursoraction.cpp",
            "findusrforcursoraction.h",
            "locationsourcefilecallbacks.cpp",
            "locationsourcefilecallbacks.h",
            "macropreprocessorcallbacks.cpp",
            "macropreprocessorcallbacks.h",
            "refactoringcompilationdatabase.cpp",
            "refactoringcompilationdatabase.h",
            "refactoringserver.cpp",
            "refactoringserver.h",
            "sourcelocationentry.h",
            "sourcelocationsutils.h",
            "sourcerangeextractor.cpp",
            "sourcerangeextractor.h",
            "sourcerangefilter.cpp",
            "sourcerangefilter.h",
            "storagesqlitestatementfactory.h",
            "symbolentry.cpp",
            "symbolentry.h",
            "symbolfinder.cpp",
            "symbolfinder.h",
            "symbolindexer.cpp",
            "symbolindexer.h",
            "symbolindexing.cpp",
            "symbolindexing.h",
            "symbolindexinginterface.h",
            "symbollocationfinderaction.cpp",
            "symbollocationfinderaction.h",
            "symbolscollector.cpp",
            "symbolscollector.h",
            "symbolscollectorinterface.h",
            "symbolstorage.cpp",
            "symbolstorage.h",
            "symbolstorageinterface.h",
        ]
    }
}
