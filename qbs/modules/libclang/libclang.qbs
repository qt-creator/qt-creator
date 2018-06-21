import qbs
import qbs.Environment
import qbs.File
import qbs.Utilities
import QtcFunctions
import "functions.js" as ClangFunctions

Module {
    Probe {
        id: clangProbe

        property stringList hostOS: qbs.hostOS
        property stringList targetOS: qbs.targetOS

        property string llvmConfig
        property string llvmVersion
        property string llvmIncludeDir
        property string llvmLibDir
        property string llvmBinDir
        property stringList llvmLibs
        property stringList llvmToolingLibs
        property stringList llvmToolingDefines
        property stringList llvmToolingIncludes
        property stringList llvmToolingCxxFlags
        property string llvmBuildMode

        configure: {
            llvmConfig = ClangFunctions.llvmConfig(hostOS, QtcFunctions);
            llvmVersion = ClangFunctions.version(llvmConfig);
            llvmIncludeDir = ClangFunctions.includeDir(llvmConfig);
            llvmLibDir = ClangFunctions.libDir(llvmConfig);
            llvmBinDir = ClangFunctions.binDir(llvmConfig);
            llvmLibs = ClangFunctions.libraries(targetOS);
            llvmToolingLibs = ClangFunctions.toolingLibs(llvmConfig, targetOS);
            llvmBuildMode = ClangFunctions.buildMode(llvmConfig);
            var toolingParams = ClangFunctions.toolingParameters(llvmConfig);
            llvmToolingDefines = toolingParams.defines;
            llvmToolingIncludes = toolingParams.includes;
            llvmToolingCxxFlags = toolingParams.cxxFlags;
            found = llvmConfig && File.exists(llvmIncludeDir.concat("/clang-c/Index.h"));
        }
    }

    property string llvmConfig: clangProbe.llvmConfig
    property string llvmVersion: clangProbe.llvmVersion
    property string llvmIncludeDir: clangProbe.llvmIncludeDir
    property string llvmLibDir: clangProbe.llvmLibDir
    property string llvmBinDir: clangProbe.llvmBinDir
    property stringList llvmLibs: clangProbe.llvmLibs
    property stringList llvmToolingLibs: clangProbe.llvmToolingLibs
    property string llvmBuildMode: clangProbe.llvmBuildMode
    property bool llvmBuildModeMatches: qbs.buildVariant === llvmBuildMode.toLowerCase()
    property stringList llvmToolingDefines: clangProbe.llvmToolingDefines
    property stringList llvmToolingIncludes: clangProbe.llvmToolingIncludes.filter(function(incl) {
        return incl != llvmIncludeDir;
    })
    property stringList llvmToolingCxxFlags: clangProbe.llvmToolingCxxFlags
    property bool toolingEnabled: !Environment.getEnv("QTC_NO_CLANG_LIBTOOLING")
                                  && Utilities.versionCompare(llvmVersion, "6") > 0
                                  && Utilities.versionCompare(llvmVersion, "7") < 0

    validate: {
        if (!clangProbe.found) {
            console.warn("No usable libclang version found."
                         + " Set LLVM_INSTALL_DIR to build the Clang Code Model."
                         + " For details, see doc/src/editors/creator-clang-codemodel.qdoc.");
            throw new Error();
        }
    }
}
