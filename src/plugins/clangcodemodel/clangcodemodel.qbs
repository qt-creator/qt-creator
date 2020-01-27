import qbs
import qbs.FileInfo

QtcPlugin {
    name: "ClangCodeModel"

    Depends { name: "Qt"; submodules: ["concurrent", "widgets"] }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "ClangSupport" }

    Depends { name: "libclang"; required: false }
    Depends { name: "clang_defines" }

    pluginTestDepends: [
        "CppEditor",
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
        "clangdiagnosticfilter.cpp",
        "clangdiagnosticfilter.h",
        "clangdiagnosticmanager.cpp",
        "clangdiagnosticmanager.h",
        "clangdiagnostictooltipwidget.cpp",
        "clangdiagnostictooltipwidget.h",
        "clangeditordocumentparser.cpp",
        "clangeditordocumentparser.h",
        "clangeditordocumentprocessor.cpp",
        "clangeditordocumentprocessor.h",
        "clangfixitoperation.cpp",
        "clangfixitoperation.h",
        "clangfixitoperationsextractor.cpp",
        "clangfixitoperationsextractor.h",
        "clangfollowsymbol.cpp",
        "clangfollowsymbol.h",
        "clangfunctionhintmodel.cpp",
        "clangfunctionhintmodel.h",
        "clanghighlightingresultreporter.cpp",
        "clanghighlightingresultreporter.h",
        "clanghoverhandler.cpp",
        "clanghoverhandler.h",
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
