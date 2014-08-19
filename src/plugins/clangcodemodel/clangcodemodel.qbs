import qbs
import qbs.File
import qbs.Process
import QtcProcessOutputReader

QtcPlugin {
    name: "ClangCodeModel"

    Depends { name: "Qt"; submodules: ["concurrent", "widgets"] }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }

    property string llvmInstallDirFromEnv: qbs.getEnv("LLVM_INSTALL_DIR")

    property bool clangCompletion: true
    property bool clangHighlighting: true
    property bool clangIndexing: false

    property string llvmConfig: {
        var llvmConfigVariants = [
            "llvm-config", "llvm-config-3.2", "llvm-config-3.3", "llvm-config-3.4",
            "llvm-config-3.5", "llvm-config-3.6", "llvm-config-4.0", "llvm-config-4.1"
        ];

        // Prefer llvm-config* from LLVM_INSTALL_DIR
        if (llvmInstallDirFromEnv) {
            for (var i = 0; i < llvmConfigVariants.length; ++i) {
                var variant = llvmInstallDirFromEnv + "/bin/" + llvmConfigVariants[i];
                if (File.exists(variant))
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
                if (File.exists(variant))
                    return variant;
            }
        }

        return undefined;
    }
    condition: llvmConfig

    property string llvmIncludeDir: QtcProcessOutputReader.readOutput(llvmConfig, ["--includedir"])
    property string llvmLibDir: QtcProcessOutputReader.readOutput(llvmConfig, ["--libdir"])
    property string llvmVersion: QtcProcessOutputReader.readOutput(llvmConfig, ["--version"])
        .replace(/(\d+\.\d+).*/, "$1")

    cpp.includePaths: base.concat(llvmIncludeDir)
    cpp.libraryPaths: base.concat(llvmLibDir)
    cpp.rpaths: cpp.libraryPaths

    property string llvmLib: "clang"
    property stringList additionalLibraries: qbs.targetOS.contains("windows")
                                             ? ["advapi32", "shell32"] : []
    cpp.dynamicLibraries: base.concat(llvmLib).concat(additionalLibraries)

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
            "clangsymbolsearcher.cpp",
            "clangsymbolsearcher.h",
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
        ]
    }

    Group {
        name: "Test resources"
        prefix: "test/"
        fileTags: "none"
        files: [
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
