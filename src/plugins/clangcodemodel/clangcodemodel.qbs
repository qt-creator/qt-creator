import qbs
import qbs.File
import QtcClangInstallation as Clang
import QtcFunctions
import QtcProcessOutputReader

QtcPlugin {
    name: "ClangCodeModel"

    Depends { name: "Qt"; submodules: ["concurrent", "widgets"] }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "ClangBackEndIpc" }

    pluginTestDepends: [
        "CppEditor",
        "QmakeProjectManager",
    ]

    property string llvmConfig: Clang.llvmConfig(qbs, QtcFunctions, QtcProcessOutputReader)
    property string llvmIncludeDir: Clang.includeDir(llvmConfig, QtcProcessOutputReader)
    property string llvmLibDir: Clang.libDir(llvmConfig, QtcProcessOutputReader)
    property string llvmVersion: Clang.version(llvmConfig, QtcProcessOutputReader)

    condition: llvmConfig && File.exists(llvmIncludeDir.concat("/clang-c/Index.h"))

    cpp.defines: {
        var defines = base;
        // The following defines are used to determine the clang include path for intrinsics.
        defines.push('CLANG_VERSION="' + llvmVersion + '"');
        defines.push('CLANG_RESOURCE_DIR="' + llvmLibDir + '/clang/' + llvmVersion + '/include"');
        return defines;
    }

    files: [
        "clangactivationsequencecontextprocessor.cpp",
        "clangactivationsequencecontextprocessor.h",
        "clangactivationsequenceprocessor.cpp",
        "clangactivationsequenceprocessor.h",
        "clangassistproposal.cpp",
        "clangassistproposal.h",
        "clangassistproposalitem.cpp",
        "clangassistproposalitem.h",
        "clangassistproposalmodel.cpp",
        "clangassistproposalmodel.h",
        "clangbackendipcintegration.cpp",
        "clangbackendipcintegration.h",
        "clangcodemodelplugin.cpp",
        "clangcodemodelplugin.h",
        "clangcompletionassistinterface.cpp",
        "clangcompletionassistinterface.h",
        "clangcompletionassistprocessor.cpp",
        "clangcompletionassistprocessor.h",
        "clangcompletionassistprovider.cpp",
        "clangcompletionassistprovider.h",
        "clangcompletionchunkstotextconverter.cpp",
        "clangcompletionchunkstotextconverter.h",
        "clangcompletioncontextanalyzer.cpp",
        "clangcompletioncontextanalyzer.h",
        "clangconstants.h",
        "clangdiagnosticfilter.cpp",
        "clangdiagnosticfilter.h",
        "clangdiagnosticmanager.cpp",
        "clangdiagnosticmanager.h",
        "clangeditordocumentparser.cpp",
        "clangeditordocumentparser.h",
        "clangeditordocumentprocessor.cpp",
        "clangeditordocumentprocessor.h",
        "clangfixitoperation.cpp",
        "clangfixitoperation.h",
        "clangfixitoperationsextractor.cpp",
        "clangfixitoperationsextractor.h",
        "clangfunctionhintmodel.cpp",
        "clangfunctionhintmodel.h",
        "clanghighlightingmarksreporter.cpp",
        "clanghighlightingmarksreporter.h",
        "clangmodelmanagersupport.cpp",
        "clangmodelmanagersupport.h",
        "clangtextmark.cpp",
        "clangtextmark.h",
        "clangutils.cpp",
        "clangutils.h",
    ]

    Group {
        name: "Tests"
        condition: project.testsEnabled
        prefix: "test/"
        files: [
            "data/clangtestdata.qrc",
            "clangcodecompletion_test.cpp",
            "clangcodecompletion_test.h",
        ]
    }

    Group {
        name: "Test resources"
        prefix: "test/data/"
        fileTags: "none"
        files: [ "*" ]
        excludeFiles: "clangtestdata.qrc"
    }
}
