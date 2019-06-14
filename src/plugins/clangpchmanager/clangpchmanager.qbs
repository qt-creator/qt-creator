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
        "clangindexingprojectsettings.cpp",
        "clangindexingprojectsettings.h",
        "clangindexingprojectsettingswidget.cpp",
        "clangindexingprojectsettingswidget.h",
        "clangindexingprojectsettingswidget.ui",
        "clangindexingsettingsmanager.cpp",
        "clangindexingsettingsmanager.h",
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
        "preprocessormacrocollector.cpp",
        "preprocessormacrocollector.h",
        "preprocessormacrowidget.cpp",
        "preprocessormacrowidget.h",
        "projectupdater.cpp",
        "projectupdater.h",
        "qtcreatorprojectupdater.cpp",
        "qtcreatorprojectupdater.h",
    ]
}
