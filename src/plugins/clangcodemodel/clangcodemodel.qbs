import qbs
import qbs.FileInfo

QtcPlugin {
    name: "ClangCodeModel"

    Depends { name: "Qt"; submodules: ["concurrent", "widgets"] }

    Depends { name: "ClangSupport" }
    Depends { name: "Core" }
    Depends { name: "CppEditor" }
    Depends { name: "LanguageClient" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport"; condition: qtc.testsEnabled }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }

    Depends { name: "libclang"; required: false }
    Depends { name: "clang_defines" }

    pluginTestDepends: [
        "QmakeProjectManager",
    ]

    condition: libclang.present

    files: [
        "clangactivationsequencecontextprocessor.cpp",
        "clangactivationsequencecontextprocessor.h",
        "clangactivationsequenceprocessor.cpp",
        "clangactivationsequenceprocessor.h",
        "clangassistproposalitem.cpp",
        "clangassistproposalitem.h",
        "clangassistproposalmodel.cpp",
        "clangassistproposalmodel.h",
        "clangbackendcommunicator.cpp",
        "clangbackendcommunicator.h",
        "clangbackendlogging.cpp",
        "clangbackendlogging.h",
        "clangbackendreceiver.cpp",
        "clangbackendreceiver.h",
        "clangbackendsender.cpp",
        "clangbackendsender.h",
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
        "clangcurrentdocumentfilter.cpp",
        "clangcurrentdocumentfilter.h",
        "clangdclient.cpp",
        "clangdclient.h",
        "clangdiagnosticfilter.cpp",
        "clangdiagnosticfilter.h",
        "clangdiagnosticmanager.cpp",
        "clangdiagnosticmanager.h",
        "clangdiagnostictooltipwidget.cpp",
        "clangdiagnostictooltipwidget.h",
        "clangdlocatorfilters.cpp",
        "clangdlocatorfilters.h",
        "clangdqpropertyhighlighter.cpp",
        "clangdqpropertyhighlighter.h",
        "clangdquickfixfactory.cpp",
        "clangdquickfixfactory.h",
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
        "clanghighlightingresultreporter.cpp",
        "clanghighlightingresultreporter.h",
        "clangisdiagnosticrelatedtolocation.h",
        "clangmodelmanagersupport.cpp",
        "clangmodelmanagersupport.h",
        "clangoverviewmodel.cpp",
        "clangoverviewmodel.h",
        "clangpreprocessorassistproposalitem.cpp",
        "clangpreprocessorassistproposalitem.h",
        "clangprojectsettings.cpp",
        "clangprojectsettings.h",
        "clangprojectsettingswidget.cpp",
        "clangprojectsettingswidget.h",
        "clangprojectsettingswidget.ui",
        "clangrefactoringengine.cpp",
        "clangrefactoringengine.h",
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
            "clangautomationutils.cpp",
            "clangautomationutils.h",
            "clangbatchfileprocessor.cpp",
            "clangbatchfileprocessor.h",
            "clangcodecompletion_test.cpp",
            "clangcodecompletion_test.h",
            "clangdtests.cpp",
            "clangdtests.h",
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
