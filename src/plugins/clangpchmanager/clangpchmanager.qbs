import qbs
import qbs.FileInfo

QtcPlugin {
    name: "ClangPchManager"

    Depends { name: "libclang"; required: false }
    Depends { name: "clang_defines" }
    condition: libclang.present && libclang.toolingEnabled

    Depends { name: "ClangSupport" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }

    cpp.defines: base.concat("CLANGPCHMANAGER_LIB")
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
