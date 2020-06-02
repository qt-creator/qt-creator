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
        property stringList toolchain: qbs.toolchain

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
        property stringList llvmFormattingLibs
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
            if (toolchain.contains("gcc")) {
                llvmToolingCxxFlags.push("-Wno-unused-parameter");
                // clang/Format/Format.h has intentional multiline comments
                llvmToolingCxxFlags.push("-Wno-comment");
            }
            llvmFormattingLibs = ClangFunctions.formattingLibs(llvmConfig, QtcFunctions, targetOS);
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
    property stringList llvmFormattingLibs: clangProbe.llvmFormattingLibs
    property string llvmBuildMode: clangProbe.llvmBuildMode
    property bool llvmBuildModeMatches: qbs.buildVariant === llvmBuildMode.toLowerCase()
    property stringList llvmToolingDefines: clangProbe.llvmToolingDefines
    property stringList llvmToolingIncludes: clangProbe.llvmToolingIncludes.filter(function(incl) {
        return incl != llvmIncludeDir;
    })
    property stringList llvmToolingCxxFlags: clangProbe.llvmToolingCxxFlags
    property bool toolingEnabled: Environment.getEnv("QTC_ENABLE_CLANG_REFACTORING")

    validate: {
        if (!clangProbe.found) {
            console.warn("No usable libclang version found."
                         + " Set LLVM_INSTALL_DIR to build the Clang Code Model."
                         + " For details, see"
                         + " doc/src/editors/creator-only/creator-clang-codemodel.qdoc.");
            throw new Error();
        }
    }
}
