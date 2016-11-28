import qbs
import qbs.File
import QtcFunctions
import "functions.js" as ClangFunctions

Module {
    Probe {
        id: clangProbe

        property string llvmConfig
        property string llvmVersion
        property string llvmIncludeDir
        property string llvmLibDir
        property stringList llvmLibs

        configure: {
            llvmConfig = ClangFunctions.llvmConfig(qbs, QtcFunctions);
            llvmVersion = ClangFunctions.version(llvmConfig);
            llvmIncludeDir = ClangFunctions.includeDir(llvmConfig);
            llvmLibDir = ClangFunctions.libDir(llvmConfig);
            llvmLibs = ClangFunctions.libraries(qbs.targetOS);
            found = llvmConfig && File.exists(llvmIncludeDir.concat("/clang-c/Index.h"));
        }
    }

    property string llvmConfig: clangProbe.llvmConfig
    property string llvmVersion: clangProbe.llvmVersion
    property string llvmIncludeDir: clangProbe.llvmIncludeDir
    property string llvmLibDir: clangProbe.llvmLibDir
    property stringList llvmLibs: clangProbe.llvmLibs

    validate: {
        if (!clangProbe.found) {
            console.warn("Set LLVM_INSTALL_DIR to build the Clang Code Model."
                         + " For details, see doc/src/editors/creator-clang-codemodel.qdoc.");
            throw "No usable libclang found";
        }
    }
}
