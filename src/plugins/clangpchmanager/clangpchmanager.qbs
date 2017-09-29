import qbs
import qbs.FileInfo

QtcPlugin {
    name: "ClangPchManager"

    Depends { name: "libclang"; required: false }
    condition: libclang.present && libclang.toolingEnabled

    Depends { name: "ClangSupport" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }

    cpp.defines: {
        var defines = base;
        defines.push("CLANGPCHMANAGER_LIB");

        // The following defines are used to determine the clang include path for intrinsics.
        defines.push('CLANG_VERSION="' + libclang.llvmVersion + '"');
        var resourceDir = FileInfo.joinPaths(libclang.llvmLibDir, "clang", libclang.llvmVersion,
                                             "include");
        defines.push('CLANG_RESOURCE_DIR="' + resourceDir + '"');
        return defines;
    }

    cpp.includePaths: ["."]

    files: [
        "clangpchmanagerplugin.cpp",
        "clangpchmanagerplugin.h",
        "clangpchmanager_global.h",
        "pchmanagerclient.cpp",
        "pchmanagerclient.h",
        "pchmanagernotifierinterface.cpp",
        "pchmanagernotifierinterface.h",
        "pchmanagerconnectionclient.cpp",
        "pchmanagerconnectionclient.h",
        "pchmanagerprojectupdater.cpp",
        "pchmanagerprojectupdater.h",
        "projectupdater.cpp",
        "projectupdater.h",
        "qtcreatorprojectupdater.cpp",
        "qtcreatorprojectupdater.h",
    ]
}
