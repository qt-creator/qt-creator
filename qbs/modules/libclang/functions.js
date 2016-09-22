var Environment = loadExtension("qbs.Environment")
var File = loadExtension("qbs.File")
var FileInfo = loadExtension("qbs.FileInfo")
var MinimumLLVMVersion = "3.9.0"
var Process = loadExtension("qbs.Process")

function readOutput(executable, args)
{
    var p = new Process();
    var output = "";
    if (p.exec(executable, args, false) !== -1)
        output = p.readStdOut().trim(); // Trailing newline.
    p.close();
    return output;
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
