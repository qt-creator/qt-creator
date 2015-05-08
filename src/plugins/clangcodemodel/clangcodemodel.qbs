import qbs
import qbs.File
import QtcClangInstallation as Clang
import QtcProcessOutputReader

QtcPlugin {
    name: "ClangCodeModel"

    Depends { name: "Qt"; submodules: ["concurrent", "widgets"] }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "CodeModelBackEndIpc" }

    pluginTestDepends: [
        "CppEditor",
        "QmakeProjectManager",
    ]

    property bool clangCompletion: true
    property bool clangHighlighting: true
    property bool clangIndexing: false

    property string llvmConfig: Clang.llvmConfig(qbs)
    property string llvmIncludeDir: Clang.includeDir(llvmConfig, QtcProcessOutputReader)
    property string llvmLibDir: Clang.libDir(llvmConfig, QtcProcessOutputReader)
    property string llvmLibs: Clang.libraries(qbs.targetOS)
    property string llvmVersion: Clang.version(llvmConfig, QtcProcessOutputReader)

    condition: llvmConfig

    cpp.includePaths: base.concat(llvmIncludeDir)
    cpp.libraryPaths: base.concat(llvmLibDir)
    cpp.rpaths: cpp.libraryPaths
    cpp.dynamicLibraries: base.concat(llvmLibs)

    cpp.defines: {
        var defines = base;
        defines.push('CLANG_VERSION="' + llvmVersion + '"');
        defines.push('CLANG_RESOURCE_DIR="' + llvmLibDir + '/clang/' + llvmVersion + '/include"');
        if (clangCompletion)
            defines.push("CLANG_COMPLETION");
        if (clangHighlighting)
            defines.push("CLANG_HIGHLIGHTING");
        if (clangIndexing)
            defines.push("CLANG_INDEXING");
        return defines;
    }

    Group {
        name: "Completion support"
        condition: product.clangCompletion
        files: [
            "clangcompleter.cpp",
            "clangcompleter.h",
            "clangcompletion.cpp",
            "clangcompletion.h",
            "completionproposalsbuilder.cpp",
            "completionproposalsbuilder.h",
        ]
    }

    Group {
        name: "Highlighting support"
        condition: product.clangHighlighting
        files: [
            "cppcreatemarkers.cpp",
            "cppcreatemarkers.h",
        ]
    }

    Group {
        name: "Indexing support"
        condition: product.clangIndexing
        files: [
            "clangindexer.cpp",
            "clangindexer.h",
            "index.cpp",
            "index.h",
            "indexer.cpp",
            "indexer.h",
            // "dependencygraph.h",
            // "dependencygraph.cpp"
        ]
    }

    Group {
        name: "Tests"
        condition: project.testsEnabled
        prefix: "test/"
        files: [
            "clang_tests_database.qrc",
            "clangcompletion_test.cpp",
            "completiontesthelper.cpp",
            "completiontesthelper.h",
            "clangcodecompletion_test.cpp",
            "clangcodecompletion_test.h",
            "clangcompletioncontextanalyzertest.cpp",
            "clangcompletioncontextanalyzertest.h",
        ]
    }

    Group {
        name: "Test resources"
        prefix: "test/"
        fileTags: "none"
        files: [
            "mysource.cpp",
            "myheader.h",
            "completionWithProject.cpp",
            "memberCompletion.cpp",
            "doxygenKeywordsCompletion.cpp",
            "preprocessorKeywordsCompletion.cpp",
            "includeDirectiveCompletion.cpp",
            "cxx_regression_1.cpp",
            "cxx_regression_2.cpp",
            "cxx_regression_3.cpp",
            "cxx_regression_4.cpp",
            "cxx_regression_5.cpp",
            "cxx_regression_6.cpp",
            "cxx_regression_7.cpp",
            "cxx_regression_8.cpp",
            "cxx_regression_9.cpp",
            "cxx_snippets_1.cpp",
            "cxx_snippets_2.cpp",
            "cxx_snippets_3.cpp",
            "cxx_snippets_4.cpp",
            "objc_messages_1.mm",
            "objc_messages_2.mm",
            "objc_messages_3.mm",
        ]
    }

    files: [
        "clang_global.h",
        "clangcompletioncontextanalyzer.cpp",
        "clangcompletioncontextanalyzer.h",
        "clangeditordocumentparser.cpp",
        "clangeditordocumentparser.h",
        "clangeditordocumentprocessor.cpp",
        "clangeditordocumentprocessor.h",
        "clangmodelmanagersupport.cpp",
        "clangmodelmanagersupport.h",
        "clangcodemodelplugin.cpp",
        "clangcodemodelplugin.h",
        "clangprojectsettings.cpp",
        "clangprojectsettings.h",
        "clangprojectsettingspropertiespage.cpp",
        "clangprojectsettingspropertiespage.h",
        "clangprojectsettingspropertiespage.ui",
        "clangutils.cpp",
        "clangutils.h",
        "codemodelbackendipcintegration.cpp",
        "codemodelbackendipcintegration.h",
        "completionchunkstotextconverter.cpp",
        "completionchunkstotextconverter.h",
        "constants.h",
        "cxprettyprinter.cpp",
        "cxprettyprinter.h",
        "cxraii.h",
        "diagnostic.cpp",
        "diagnostic.h",
        "fastindexer.cpp",
        "fastindexer.h",
        "pchinfo.cpp",
        "pchinfo.h",
        "pchmanager.cpp",
        "pchmanager.h",
        "semanticmarker.cpp",
        "semanticmarker.h",
        "sourcelocation.cpp",
        "sourcelocation.h",
        "sourcemarker.cpp",
        "sourcemarker.h",
        "symbol.cpp",
        "symbol.h",
        "unit.cpp",
        "unit.h",
        "unsavedfiledata.cpp",
        "unsavedfiledata.h",
        "utils.cpp",
        "utils.h",
        "utils_p.cpp",
        "utils_p.h",
        "raii/scopedclangoptions.cpp",
        "raii/scopedclangoptions.h",
    ]
}
