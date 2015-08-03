var File = loadExtension("qbs.File")
var MinimumLLVMVersion = "3.6.0"

function isSuitableLLVMConfig(llvmConfigCandidate, qtcFunctions, processOutputReader)
{
    if (File.exists(llvmConfigCandidate)) {
        var candidateVersion = version(llvmConfigCandidate, processOutputReader);
        if (candidateVersion && candidateVersion.length)
            return qtcFunctions.versionIsAtLeast(candidateVersion, MinimumLLVMVersion)
    }
    return false;
}

function llvmConfig(qbs, qtcFunctions, processOutputReader)
{
    var llvmInstallDirFromEnv = qbs.getEnv("LLVM_INSTALL_DIR")
    var llvmConfigVariants = [
        "llvm-config", "llvm-config-3.2", "llvm-config-3.3", "llvm-config-3.4",
        "llvm-config-3.5", "llvm-config-3.6", "llvm-config-4.0", "llvm-config-4.1"
    ];

    // Prefer llvm-config* from LLVM_INSTALL_DIR
    if (llvmInstallDirFromEnv) {
        for (var i = 0; i < llvmConfigVariants.length; ++i) {
            var variant = llvmInstallDirFromEnv + "/bin/" + llvmConfigVariants[i];
            if (isSuitableLLVMConfig(variant, qtcFunctions, processOutputReader))
                return variant;
        }
    }

    // Find llvm-config* in PATH
    var pathListString = qbs.getEnv("PATH");
    var separator = qbs.hostOS.contains("windows") ? ";" : ":";
    var pathList = pathListString.split(separator);
    for (var i = 0; i < llvmConfigVariants.length; ++i) {
        for (var j = 0; j < pathList.length; ++j) {
            var variant = pathList[j] + "/" + llvmConfigVariants[i];
            if (isSuitableLLVMConfig(variant, qtcFunctions, processOutputReader))
                return variant;
        }
    }

    return undefined;
}

function includeDir(llvmConfig, processOutputReader)
{
    return processOutputReader.readOutput(llvmConfig, ["--includedir"])
}

function libDir(llvmConfig, processOutputReader)
{
    return processOutputReader.readOutput(llvmConfig, ["--libdir"])
}

function version(llvmConfig, processOutputReader)
{
    return processOutputReader.readOutput(llvmConfig, ["--version"])
        .replace(/(\d+\.\d+\.\d+).*/, "$1")
}

function libraries(targetOS)
{
    return ["clang"] + (targetOS.contains("windows") ? ["advapi32", "shell32"] : [])
}
