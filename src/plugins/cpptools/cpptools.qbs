import qbs 1.0
import qbs.FileInfo

Project {
    name: "CppTools"

    QtcDevHeaders { }

    QtcPlugin {
        Depends { name: "Qt.widgets" }
        Depends { name: "CPlusPlus" }
        Depends { name: "Utils" }

        Depends { name: "Core" }
        Depends { name: "TextEditor" }
        Depends { name: "ProjectExplorer" }
        Depends { name: "app_version_header" }

        pluginTestDepends: [
            "CppEditor",
            "QmakeProjectManager",
        ]

        cpp.defines: base
        Properties {
            condition: qbs.toolchain.contains("msvc")
            cpp.defines: base.concat("_SCL_SECURE_NO_WARNINGS")
        }

        files: [
            "abstracteditorsupport.cpp",
            "abstracteditorsupport.h",
            "baseeditordocumentparser.cpp",
            "baseeditordocumentparser.h",
            "baseeditordocumentprocessor.cpp",
            "baseeditordocumentprocessor.h",
            "builtineditordocumentparser.cpp",
            "builtineditordocumentparser.h",
            "builtineditordocumentprocessor.cpp",
            "builtineditordocumentprocessor.h",
            "builtinindexingsupport.cpp",
            "builtinindexingsupport.h",
            "clangcompileroptionsbuilder.cpp",
            "clangcompileroptionsbuilder.h",
            "clangdiagnosticconfig.cpp",
            "clangdiagnosticconfig.h",
            "clangdiagnosticconfigsmodel.cpp",
            "clangdiagnosticconfigsmodel.h",
            "clangdiagnosticconfigswidget.cpp",
            "clangdiagnosticconfigswidget.h",
            "clangdiagnosticconfigswidget.ui",
            "compileroptionsbuilder.cpp",
            "compileroptionsbuilder.h",
            "cppchecksymbols.cpp",
            "cppchecksymbols.h",
            "cppclassesfilter.cpp",
            "cppclassesfilter.h",
            "cppcodeformatter.cpp",
            "cppcodeformatter.h",
            "cppcodemodelinspectordumper.cpp",
            "cppcodemodelinspectordumper.h",
            "cppcodemodelsettings.cpp",
            "cppcodemodelsettings.h",
            "cppcodemodelsettingspage.cpp",
            "cppcodemodelsettingspage.h",
            "cppcodemodelsettingspage.ui",
            "cppcodestylepreferences.cpp",
            "cppcodestylepreferences.h",
            "cppcodestylepreferencesfactory.cpp",
            "cppcodestylepreferencesfactory.h",
            "cppcodestylesettings.cpp",
            "cppcodestylesettings.h",
            "cppcodestylesettingspage.cpp",
            "cppcodestylesettingspage.h",
            "cppcodestylesettingspage.ui",
            "cppcompletionassist.cpp",
            "cppcompletionassist.h",
            "cppcompletionassistprocessor.cpp",
            "cppcompletionassistprocessor.h",
            "cppcompletionassistprovider.cpp",
            "cppcompletionassistprovider.h",
            "cppcurrentdocumentfilter.cpp",
            "cppcurrentdocumentfilter.h",
            "cppdoxygen.cpp",
            "cppdoxygen.h",
            "cppeditoroutline.cpp",
            "cppeditoroutline.h",
            "cppfileiterationorder.cpp",
            "cppfileiterationorder.h",
            "cppfilesettingspage.cpp",
            "cppfilesettingspage.h",
            "cppfilesettingspage.ui",
            "cppfindreferences.cpp",
            "cppfindreferences.h",
            "cppfunctionsfilter.cpp",
            "cppfunctionsfilter.h",
            "cppincludesfilter.cpp",
            "cppincludesfilter.h",
            "cppindexingsupport.cpp",
            "cppindexingsupport.h",
            "cpplocalsymbols.cpp",
            "cpplocalsymbols.h",
            "cpplocatordata.cpp",
            "cpplocatordata.h",
            "cpplocatorfilter.cpp",
            "cpplocatorfilter.h",
            "cppmodelmanager.cpp",
            "cppmodelmanager.h",
            "cppmodelmanagersupport.cpp",
            "cppmodelmanagersupport.h",
            "cppmodelmanagersupportinternal.cpp",
            "cppmodelmanagersupportinternal.h",
            "cpppointerdeclarationformatter.cpp",
            "cpppointerdeclarationformatter.h",
            "cppprojectfile.cpp",
            "cppprojectfile.h",
            "cppprojectfilecategorizer.cpp",
            "cppprojectfilecategorizer.h",
            "cppprojectinfogenerator.cpp",
            "cppprojectinfogenerator.h",
            "cppprojectpartchooser.cpp",
            "cppprojectpartchooser.h",
            "cppprojectupdater.cpp",
            "cppprojectupdater.h",
            "cppqtstyleindenter.cpp",
            "cppqtstyleindenter.h",
            "cpprawprojectpart.cpp",
            "cpprawprojectpart.h",
            "cpprefactoringchanges.cpp",
            "cpprefactoringchanges.h",
            "cppselectionchanger.cpp",
            "cppselectionchanger.h",
            "cppsemanticinfo.cpp",
            "cppsemanticinfo.h",
            "cppsemanticinfoupdater.cpp",
            "cppsemanticinfoupdater.h",
            "cppsourceprocessor.cpp",
            "cppsourceprocessor.h",
            "cpptools.qrc",
            "cpptools_global.h",
            "cpptools_utils.h",
            "cpptoolsbridge.cpp",
            "cpptoolsbridge.h",
            "cpptoolsbridgeinterface.h",
            "cpptoolsbridgeqtcreatorimplementation.cpp",
            "cpptoolsbridgeqtcreatorimplementation.h",
            "cpptoolsconstants.h",
            "cpptoolsjsextension.cpp",
            "cpptoolsjsextension.h",
            "cpptoolsplugin.cpp",
            "cpptoolsplugin.h",
            "cpptoolsreuse.cpp",
            "cpptoolsreuse.h",
            "cpptoolssettings.cpp",
            "cpptoolssettings.h",
            "cppworkingcopy.cpp",
            "cppworkingcopy.h",
            "doxygengenerator.cpp",
            "doxygengenerator.h",
            "editordocumenthandle.cpp",
            "editordocumenthandle.h",
            "functionutils.cpp",
            "functionutils.h",
            "generatedcodemodelsupport.cpp",
            "generatedcodemodelsupport.h",
            "includeutils.cpp",
            "includeutils.h",
            "indexitem.cpp",
            "indexitem.h",
            "insertionpointlocator.cpp",
            "insertionpointlocator.h",
            "projectinfo.cpp",
            "projectinfo.h",
            "projectpart.cpp",
            "projectpart.h",
            "projectpartheaderpath.h",
            "searchsymbols.cpp",
            "searchsymbols.h",
            "semantichighlighter.cpp",
            "semantichighlighter.h",
            "senddocumenttracker.cpp",
            "senddocumenttracker.h",
            "stringtable.cpp",
            "stringtable.h",
            "symbolfinder.cpp",
            "symbolfinder.h",
            "symbolsfindfilter.cpp",
            "symbolsfindfilter.h",
            "typehierarchybuilder.cpp",
            "typehierarchybuilder.h",
        ]

        Group {
            name: "Tests"
            condition: qtc.testsEnabled
            files: [
                "cppcodegen_test.cpp",
                "cppcompletion_test.cpp",
                "cppheadersource_test.cpp",
                "cpplocalsymbols_test.cpp",
                "cpplocatorfilter_test.cpp",
                "cppmodelmanager_test.cpp",
                "cpppointerdeclarationformatter_test.cpp",
                "cppsourceprocessertesthelper.cpp",
                "cppsourceprocessertesthelper.h",
                "cppsourceprocessor_test.cpp",
                "cpptoolstestcase.cpp",
                "cpptoolstestcase.h",
                "modelmanagertesthelper.cpp",
                "modelmanagertesthelper.h",
                "symbolsearcher_test.cpp",
                "typehierarchybuilder_test.cpp",
            ]

            cpp.defines: outer.concat(['SRCDIR="' + FileInfo.path(filePath) + '"'])
        }

        Export {
            Depends { name: "CPlusPlus" }
            Depends { name: "Qt.concurrent" }
        }
    }
}
