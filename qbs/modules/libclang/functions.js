var Environment = require("qbs.Environment")
var File = require("qbs.File")
var FileInfo = require("qbs.FileInfo")
var MinimumLLVMVersion = "8.0.0" // CLANG-UPGRADE-CHECK: Adapt minimum version numbers.
var Process = require("qbs.Process")
var Utilities = require("qbs.Utilities")

function readOutput(executable, args)
{
    var p = new Process();
    var output = "";
    if (p.exec(executable, args, false) !== -1)
        output = p.readStdOut().trim(); // Trailing newline.
    p.close();
    return output;
}

function readListOutput(executable, args)
{
    var list = readOutput(executable, args).split(/\s+/);
    if (!list[list.length - 1])
        list.pop();
    return list;
}

function isSuitableLLVMConfig(llvmConfigCandidate, qtcFunctions)
{
    if (File.exists(llvmConfigCandidate)) {
        var candidateVersion = version(llvmConfigCandidate);
        if (candidateVersion && candidateVersion.length)
            return qtcFunctions.versionIsAtLeast(candidateVersion, MinimumLLVMVersion)
    }
    return false;
}

function llvmConfig(hostOS, qtcFunctions)
{
    var llvmInstallDirFromEnv = Environment.getEnv("LLVM_INSTALL_DIR")
    var llvmConfigVariants = [
        // CLANG-UPGRADE-CHECK: Adapt once we require a new minimum version.
        "llvm-config", "llvm-config-8", "llvm-config-9", "llvm-config-10", "llvm-config-11", "llvm-config-12"
    ];

    // Prefer llvm-config* from LLVM_INSTALL_DIR
    var suffix = hostOS.contains("windows") ? ".exe" : "";
    if (llvmInstallDirFromEnv) {
        for (var i = 0; i < llvmConfigVariants.length; ++i) {
            var variant = llvmInstallDirFromEnv + "/bin/" + llvmConfigVariants[i] + suffix;
            if (isSuitableLLVMConfig(variant, qtcFunctions))
                return variant;
        }
    }

    // Find llvm-config* in PATH
    var pathListString = Environment.getEnv("PATH");
    var separator = hostOS.contains("windows") ? ";" : ":";
    var pathList = pathListString.split(separator);
    for (var i = 0; i < llvmConfigVariants.length; ++i) {
        for (var j = 0; j < pathList.length; ++j) {
            var variant = pathList[j] + "/" + llvmConfigVariants[i] + suffix;
            if (isSuitableLLVMConfig(variant, qtcFunctions))
                return variant;
        }
    }

    return undefined;
}

function includeDir(llvmConfig)
{
    return FileInfo.fromNativeSeparators(readOutput(llvmConfig, ["--includedir"]));
}

function libDir(llvmConfig)
{
    return FileInfo.fromNativeSeparators(readOutput(llvmConfig, ["--libdir"]));
}

function binDir(llvmConfig)
{
    return FileInfo.fromNativeSeparators(readOutput(llvmConfig, ["--bindir"]));
}

function version(llvmConfig)
{
    return readOutput(llvmConfig, ["--version"]).replace(/(\d+\.\d+\.\d+).*/, "$1")
}

function libraries(targetOS)
{
    return targetOS.contains("windows") ? ["libclang.lib", "advapi32.lib", "shell32.lib"] : ["clang"]
}

function extraLibraries(llvmConfig, targetOS)
{
    var libs = []
    if (targetOS.contains("windows"))
        libs.push("version");
    var dynamicList = readListOutput(llvmConfig, ["--libs"])
        .concat(readListOutput(llvmConfig, ["--system-libs"]));
    return libs.concat(dynamicList.map(function(s) {
        return s.startsWith("-l") ? s.slice(2) : s;
    }));
}

function formattingLibs(llvmConfig, qtcFunctions, targetOS)
{
    var llvmIncludeDir = includeDir(llvmConfig);
    if (!File.exists(llvmIncludeDir.concat("/clang/Format/Format.h")))
        return [];

    var clangVersion = version(llvmConfig)
    var libs = []
    if (qtcFunctions.versionIsAtLeast(clangVersion, MinimumLLVMVersion)) {
        var hasLibClangFormat = File.directoryEntries(libDir(llvmConfig), File.Files)
                .some(function(p) { return p.contains("clangFormat"); });
        if (hasLibClangFormat) {
            libs.push(
                "clangFormat",
                "clangToolingInclusions",
                "clangToolingCore",
                "clangRewrite",
                "clangLex",
                "clangBasic"
            );
        } else {
            libs.push("clang-cpp");
        }
        libs = libs.concat(extraLibraries(llvmConfig, targetOS));
    }

    return libs;
}

function toolingLibs(llvmConfig, targetOS)
{
    var hasLibClangTooling = File.directoryEntries(libDir(llvmConfig), File.Files)
            .some(function(p) { return p.contains("clangTooling"); });
    var fixedList = hasLibClangTooling ? [
        "clangTooling",
        "clangFrontend",
        "clangIndex",
        "clangParse",
        "clangSerialization",
        "clangSema",
        "clangEdit",
        "clangAnalysis",
        "clangDriver",
        "clangDynamicASTMatchers",
        "clangASTMatchers",
        "clangToolingCore",
        "clangAST",
        "clangLex",
        "clangBasic",
    ] : ["clang-cpp"];

    return fixedList.concat(extraLibraries(llvmConfig, targetOS));
}

function toolingParameters(llvmConfig)
{
    var params = {
        defines: [],
        includes: [],
        cxxFlags: [],
    };
    var allCxxFlags = readListOutput(llvmConfig, ["--cxxflags"]);
    var badFlags = [
        "-fno-exceptions",
        "/W4",
        "-Wcovered-switch-default",
        "-Wnon-virtual-dtor",
        "-Woverloaded-virtual",
        "-Wmissing-field-initializers",
        "-Wno-unknown-warning",
        "-Wno-unused-command-line-argument",
        "-fPIC",
        "-pedantic",
        "-Wstring-conversion",
        "-gsplit-dwarf"
    ]
    for (var i = 0; i < allCxxFlags.length; ++i) {
        var flag = allCxxFlags[i];
        if (flag.startsWith("-D") || flag.startsWith("/D")) {
            params.defines.push(flag.slice(2));
            continue;
        }
        if (flag.startsWith("-I") || flag.startsWith("/I")) {
            params.includes.push(flag.slice(2));
            continue;
        }
        if (!flag.startsWith("-std") && !flag.startsWith("-O") && !flag.startsWith("/O")
                && !flag.startsWith("-march")
                && !flag.startsWith("-Werror=")
                && !flag.startsWith("/EH")
                && !badFlags.contains(flag)) {
            params.cxxFlags.push(flag);
        }
    }
    return params;
}

function buildMode(llvmConfig)
{
    return readOutput(llvmConfig, ["--build-mode"]);
}
