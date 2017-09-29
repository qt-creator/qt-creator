import qbs
import qbs.FileInfo

QtcPlugin {
    name: "ClangRefactoring"

    Depends { name: "libclang"; required: false }
    condition: libclang.present && libclang.toolingEnabled

    Depends { name: "ClangSupport" }
    Depends { name: "Utils" }

    Depends { name: "ClangPchManager" }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }

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
        "baseclangquerytexteditorwidget.cpp",
        "baseclangquerytexteditorwidget.h",
        "clangqueryexamplehighlighter.cpp",
        "clangqueryexamplehighlighter.h",
        "clangqueryexamplehighlightmarker.h",
        "clangqueryexampletexteditorwidget.cpp",
        "clangqueryexampletexteditorwidget.h",
        "clangqueryhighlighter.cpp",
        "clangqueryhighlighter.h",
        "clangqueryhighlightmarker.h",
        "clangqueryhoverhandler.cpp",
        "clangqueryhoverhandler.h",
        "clangqueryprojectsfindfilter.cpp",
        "clangqueryprojectsfindfilter.h",
        "clangqueryprojectsfindfilter.ui",
        "clangqueryprojectsfindfilterwidget.cpp",
        "clangqueryprojectsfindfilterwidget.h",
        "clangquerytexteditorwidget.cpp",
        "clangquerytexteditorwidget.h",
        "clangrefactoringplugin.cpp",
        "clangrefactoringplugin.h",
        "projectpartutilities.cpp",
        "projectpartutilities.h",
        "qtcreatorclangqueryfindfilter.cpp",
        "qtcreatorclangqueryfindfilter.h",
        "qtcreatorsearch.cpp",
        "qtcreatorsearch.h",
        "qtcreatorsearchhandle.cpp",
        "qtcreatorsearchhandle.h",
        "querysqlitestatementfactory.h",
        "refactoringclient.cpp",
        "refactoringclient.h",
        "refactoringconnectionclient.cpp",
        "refactoringconnectionclient.h",
        "refactoringengine.cpp",
        "refactoringengine.h",
        "refactoringprojectupdater.cpp",
        "refactoringprojectupdater.h",
        "searchhandle.cpp",
        "searchhandle.h",
        "searchinterface.cpp",
        "searchinterface.h",
        "sourcelocations.h",
        "symbolquery.cpp",
        "symbolquery.h",
    ]
}
