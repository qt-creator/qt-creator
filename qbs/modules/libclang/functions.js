var Environment = require("qbs.Environment")
var File = require("qbs.File")
var FileInfo = require("qbs.FileInfo")
var MinimumLLVMVersion = "3.9.0" // CLANG-UPGRADE-CHECK: Adapt minimum version numbers.
var Process = require("qbs.Process")

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
    return readOutput(executable, args).split(/\s+/);
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

function llvmConfig(qbs, qtcFunctions)
{
    var llvmInstallDirFromEnv = Environment.getEnv("LLVM_INSTALL_DIR")
    var llvmConfigVariants = [
        "llvm-config", "llvm-config-3.9", "llvm-config-4.0", "llvm-config-4.1"
    ];

    // Prefer llvm-config* from LLVM_INSTALL_DIR
    var suffix = qbs.hostOS.contains("windows") ? ".exe" : "";
    if (llvmInstallDirFromEnv) {
        for (var i = 0; i < llvmConfigVariants.length; ++i) {
            var variant = llvmInstallDirFromEnv + "/bin/" + llvmConfigVariants[i] + suffix;
            if (isSuitableLLVMConfig(variant, qtcFunctions))
                return variant;
        }
    }

    // Find llvm-config* in PATH
    var pathListString = Environment.getEnv("PATH");
    var separator = qbs.hostOS.contains("windows") ? ";" : ":";
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

function version(llvmConfig)
{
    return readOutput(llvmConfig, ["--version"]).replace(/(\d+\.\d+\.\d+).*/, "$1")
}

function libraries(targetOS)
{
    return targetOS.contains("windows") ? ["libclang.lib", "advapi32.lib", "shell32.lib"] : ["clang"]
}

function toolingLibs(llvmConfig, targetOS)
{
    var fixedList = [
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
    ];
    if (targetOS.contains("windows"))
        fixedList.push("version");
    var dynamicList = readListOutput(llvmConfig, ["--libs"])
        .concat(readListOutput(llvmConfig, ["--system-libs"]));
    return fixedList.concat(dynamicList.map(function(s) {
        return s.startsWith("-l") ? s.slice(2) : s;
    }));
}

function toolingParameters(llvmConfig)
{
    var params = {
        defines: [],
        includes: [],
        cxxFlags: [],
    };
    var allCxxFlags = readListOutput(llvmConfig, ["--cxxflags"]);
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
                && !flag.startsWith("/EH") && flag !== "-fno-exceptions"
                && flag !== "/W4" && flag !== "-Werror=date-time"
                && flag !== "-Wcovered-switch-default" && flag !== "-fPIC" && flag !== "-pedantic"
                && flag !== "-Wstring-conversion" && flag !== "-gsplit-dwarf") {
            params.cxxFlags.push(flag);
        }
    }
    return params;
}

function buildMode(llvmConfig)
{
    return readOutput(llvmConfig, ["--build-mode"]);
}
