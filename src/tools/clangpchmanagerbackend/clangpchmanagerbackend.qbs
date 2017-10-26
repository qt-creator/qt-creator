import qbs
import qbs.FileInfo

QtcTool {
    name: "clangpchmanagerbackend"
    Depends { name: "libclang"; required: false }
    condition: libclang.present
               && libclang.toolingEnabled
               && (!qbs.targetOS.contains("windows") || libclang.llvmBuildModeMatches)

    Depends { name: "ClangSupport" }

    Depends { name: "Qt.network" }

    cpp.cxxFlags: base.concat(libclang.llvmToolingCxxFlags)
    cpp.defines: {
        var list = base.concat(libclang.llvmToolingDefines);
        list.push('CLANG_COMPILER_PATH="'
                  + FileInfo.joinPaths(FileInfo.path(libclang.llvmConfig), "clang") + '"');
        return list;
    }
    cpp.includePaths: base.concat(libclang.llvmIncludeDir).concat(libclang.llvmToolingIncludes)
                          .concat(["source", "../clangrefactoringbackend/source"])
    cpp.libraryPaths: base.concat(libclang.llvmLibDir)
    cpp.dynamicLibraries: base.concat(libclang.llvmToolingLibs)

    Properties {
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("macos")
        cpp.rpaths: base.concat(libclang.llvmLibDir)
    }

    files: [
        "clangpchmanagerbackendmain.cpp",
    ]

    Group {
        prefix: "source/"
        files: [
            "changedfilepathcompressor.h",
            "clangpathwatcherinterface.cpp",
            "clangpathwatcherinterface.h",
            "clangpathwatchernotifier.cpp",
            "clangpathwatchernotifier.h",
            "clangpathwatcher.h",
            "clangpchmanagerbackend_global.h",
            "collectincludesaction.h",
            "collectincludespreprocessorcallbacks.h",
            "collectincludestoolaction.h",
            "environment.h",
            "idpaths.h",
            "includecollector.cpp",
            "includecollector.h",
            "pchcreatorinterface.cpp",
            "pchcreatorinterface.h",
            "pchcreator.cpp",
            "pchcreator.h",
            "pchgenerator.h",
            "pchgeneratorinterface.cpp",
            "pchgeneratorinterface.h",
            "pchgeneratornotifierinterface.cpp",
            "pchgeneratornotifierinterface.h",
            "pchmanagerserver.cpp",
            "pchmanagerserver.h",
            "pchnotcreatederror.h",
            "projectpartsinterface.cpp",
            "projectpartsinterface.h",
            "projectparts.cpp",
            "projectparts.h",
        ]
    }

    Group {
        name: "sources from clangrefactoring"
        prefix: "../clangrefactoringbackend/source/"
        files: [
            "clangtool.cpp",
            "refactoringcompilationdatabase.cpp",
        ]
    }
}

