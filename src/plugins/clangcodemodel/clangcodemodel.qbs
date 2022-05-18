import qbs
import qbs.FileInfo

QtcPlugin {
    name: "ClangCodeModel"

    Depends { name: "Qt"; submodules: ["concurrent", "widgets"] }

    Depends { name: "Core" }
    Depends { name: "CppEditor" }
    Depends { name: "LanguageClient" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport"; condition: qtc.testsEnabled }
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
        "clangcompletioncontextanalyzer.cpp",
        "clangcompletioncontextanalyzer.h",
        "clangconstants.h",
        "clangdast.cpp",
        "clangdast.h",
        "clangdclient.cpp",
        "clangdclient.h",
        "clangdiagnostictooltipwidget.cpp",
        "clangdiagnostictooltipwidget.h",
        "clangdlocatorfilters.cpp",
        "clangdlocatorfilters.h",
        "clangdqpropertyhighlighter.cpp",
        "clangdqpropertyhighlighter.h",
        "clangdquickfixfactory.cpp",
        "clangdquickfixfactory.h",
        "clangeditordocumentprocessor.cpp",
        "clangeditordocumentprocessor.h",
        "clangfixitoperation.cpp",
        "clangfixitoperation.h",
        "clangmodelmanagersupport.cpp",
        "clangmodelmanagersupport.h",
        "clangpreprocessorassistproposalitem.cpp",
        "clangpreprocessorassistproposalitem.h",
        "clangprojectsettings.cpp",
        "clangprojectsettings.h",
        "clangprojectsettingswidget.cpp",
        "clangprojectsettingswidget.h",
        "clangprojectsettingswidget.ui",
        "clangtextmark.cpp",
        "clangtextmark.h",
        "clanguiheaderondiskmanager.cpp",
        "clanguiheaderondiskmanager.h",
        "clangutils.cpp",
        "clangutils.h",
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

    Group {
        name: "Tests"
        condition: qtc.testsEnabled
        prefix: "test/"
        files: [
            "clangbatchfileprocessor.cpp",
            "clangbatchfileprocessor.h",
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
