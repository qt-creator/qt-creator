import qbs
import qbs.FileInfo

Module {
    Depends { name: "cpp" }
    Depends { name: "libclang"; required: false }

    cpp.defines: libclang.present ? [
        'CLANG_VERSION="' + libclang.llvmVersion + '"',
        'CLANG_RESOURCE_DIR="' + FileInfo.joinPaths(libclang.llvmLibDir, "clang",
                                                    libclang.llvmVersion, "include") + '"',
        'CLANG_BINDIR="' + libclang.llvmBinDir + '"',
    ] : [
        'CLANG_VERSION=""',
        'CLANG_RESOURCE_DIR=""',
        'CLANG_BINDIR=""',
    ]
}
