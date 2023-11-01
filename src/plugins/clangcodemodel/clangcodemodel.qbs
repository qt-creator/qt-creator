import qbs
import qbs.FileInfo

QtcPlugin {
    name: "ClangCodeModel"

    Depends { name: "Qt"; submodules: ["concurrent", "widgets"] }

    Depends { name: "Core" }
    Depends { name: "CppEditor" }
    Depends { name: "LanguageClient" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport"; condition: qtc.withPluginTests }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }

    Depends { name: "clang_defines" }

    pluginTestDepends: [
        "QmakeProjectManager",
    ]

    files: [
        "clangactivationsequencecontextprocessor.cpp",
        "clangactivationsequencecontextprocessor.h",
        "clangactivationsequenceprocessor.cpp",
        "clangactivationsequenceprocessor.h",
        "clangcodemodelplugin.cpp",
        "clangcodemodelplugin.h",
        "clangcodemodeltr.h",
        "clangcompletioncontextanalyzer.cpp",
        "clangcompletioncontextanalyzer.h",
        "clangconstants.h",
        "clangdast.cpp",
        "clangdast.h",
        "clangdclient.cpp",
        "clangdclient.h",
        "clangdcompletion.cpp",
        "clangdcompletion.h",
        "clangdfindreferences.cpp",
        "clangdfindreferences.h",
        "clangdfollowsymbol.cpp",
        "clangdfollowsymbol.h",
        "clangdiagnostictooltipwidget.cpp",
        "clangdiagnostictooltipwidget.h",
        "clangdlocatorfilters.cpp",
        "clangdlocatorfilters.h",
        "clangdmemoryusagewidget.cpp",
        "clangdmemoryusagewidget.h",
        "clangdqpropertyhighlighter.cpp",
        "clangdqpropertyhighlighter.h",
        "clangdquickfixes.cpp",
        "clangdquickfixes.h",
        "clangdsemantichighlighting.cpp",
        "clangdsemantichighlighting.h",
        "clangdswitchdecldef.cpp",
        "clangdswitchdecldef.h",
        "clangeditordocumentprocessor.cpp",
        "clangeditordocumentprocessor.h",
        "clangfixitoperation.cpp",
        "clangfixitoperation.h",
        "clangmodelmanagersupport.cpp",
        "clangmodelmanagersupport.h",
        "clangpreprocessorassistproposalitem.cpp",
        "clangpreprocessorassistproposalitem.h",
        "clangtextmark.cpp",
        "clangtextmark.h",
        "clangutils.cpp",
        "clangutils.h",
        "tasktimers.cpp",
        "tasktimers.h",
    ]

    Group {
        name: "moc sources"
        prefix: "moc/"
        files: [
            "parser.cpp",
            "parser.h",
            "preprocessor.cpp",
            "preprocessor.h",
            "symbols.h",
            "token.cpp",
            "token.h",
            "utils.h",
        ]
        Group {
            name: "weirdly-named moc headers"
            files: [
                "keywords.cpp",
                "ppkeywords.cpp",
            ]
            fileTags: "hpp"
        }
    }

    QtcTestFiles {
        prefix: "test/"
        files: [
            "activationsequenceprocessortest.cpp",
            "activationsequenceprocessortest.h",
            "clangdtests.cpp",
            "clangdtests.h",
            "clangfixittest.cpp",
            "clangfixittest.h",
            "data/clangtestdata.qrc",
        ]
    }

    Group {
        name: "Test resources"
        prefix: "test/data/"
        fileTags: []
        files: [ "*" ]
        excludeFiles: "clangtestdata.qrc"
    }

    Group {
        name: "Other files"
        fileTags: []
        files: [
            "README",
            project.ide_source_tree + "/doc/qtcreator/src/editors/creator-only/creator-clang-codemodel.qdoc",
        ]
    }
}
