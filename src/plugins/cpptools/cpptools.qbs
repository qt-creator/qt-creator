import qbs 1.0
import qbs.FileInfo

QtcPlugin {
    name: "CppTools"

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
        "abstracteditorsupport.cpp", "abstracteditorsupport.h",
        "baseeditordocumentparser.cpp", "baseeditordocumentparser.h",
        "baseeditordocumentprocessor.cpp", "baseeditordocumentprocessor.h",
        "builtineditordocumentparser.cpp", "builtineditordocumentparser.h",
        "builtineditordocumentprocessor.cpp", "builtineditordocumentprocessor.h",
        "builtinindexingsupport.cpp", "builtinindexingsupport.h",
        "commentssettings.cpp", "commentssettings.h",
        "completionsettingspage.cpp", "completionsettingspage.h", "completionsettingspage.ui",
        "cppchecksymbols.cpp", "cppchecksymbols.h",
        "cppclassesfilter.cpp", "cppclassesfilter.h",
        "cppcodeformatter.cpp", "cppcodeformatter.h",
        "cppcodemodelinspectordumper.cpp", "cppcodemodelinspectordumper.h",
        "cppcodemodelsettings.cpp", "cppcodemodelsettings.h",
        "cppcodemodelsettingspage.cpp", "cppcodemodelsettingspage.h", "cppcodemodelsettingspage.ui",
        "cppcodestylepreferences.cpp", "cppcodestylepreferences.h",
        "cppcodestylepreferencesfactory.cpp", "cppcodestylepreferencesfactory.h",
        "cppcodestylesettings.cpp", "cppcodestylesettings.h",
        "cppcodestylesettingspage.cpp", "cppcodestylesettingspage.h", "cppcodestylesettingspage.ui",
        "cppcompletionassist.cpp", "cppcompletionassist.h",
        "cppcompletionassistprocessor.cpp", "cppcompletionassistprocessor.h",
        "cppcompletionassistprovider.cpp", "cppcompletionassistprovider.h",
        "cppcurrentdocumentfilter.cpp", "cppcurrentdocumentfilter.h",
        "cppdoxygen.cpp", "cppdoxygen.h",
        "cppeditoroutline.cpp", "cppeditoroutline.h",
        "cppfilesettingspage.cpp", "cppfilesettingspage.h", "cppfilesettingspage.ui",
        "cppfindreferences.cpp", "cppfindreferences.h",
        "cppfunctionsfilter.cpp", "cppfunctionsfilter.h",
        "cppincludesfilter.cpp", "cppincludesfilter.h",
        "cppindexingsupport.cpp", "cppindexingsupport.h",
        "cpplocalsymbols.cpp", "cpplocalsymbols.h",
        "cpplocatordata.cpp", "cpplocatordata.h",
        "cpplocatorfilter.cpp", "cpplocatorfilter.h",
        "cppmodelmanager.cpp", "cppmodelmanager.h",
        "cppmodelmanagersupport.cpp", "cppmodelmanagersupport.h",
        "cppmodelmanagersupportinternal.cpp", "cppmodelmanagersupportinternal.h",
        "cpppointerdeclarationformatter.cpp", "cpppointerdeclarationformatter.h",
        "cppprojectfile.cpp", "cppprojectfile.h",
        "cppprojects.cpp", "cppprojects.h",
        "cppqtstyleindenter.cpp", "cppqtstyleindenter.h",
        "cpprefactoringchanges.cpp", "cpprefactoringchanges.h",
        "cppsemanticinfo.cpp", "cppsemanticinfo.h",
        "cppsemanticinfoupdater.cpp", "cppsemanticinfoupdater.h",
        "cppsourceprocessor.cpp", "cppsourceprocessor.h",
        "cpptools.qrc",
        "cpptools_global.h",
        "cpptoolsconstants.h",
        "cpptoolsjsextension.cpp", "cpptoolsjsextension.h",
        "cpptoolsplugin.cpp", "cpptoolsplugin.h",
        "cpptoolsreuse.cpp", "cpptoolsreuse.h",
        "cpptoolssettings.cpp", "cpptoolssettings.h",
        "cppworkingcopy.cpp", "cppworkingcopy.h",
        "doxygengenerator.cpp", "doxygengenerator.h",
        "editordocumenthandle.cpp", "editordocumenthandle.h",
        "functionutils.cpp", "functionutils.h",
        "includeutils.cpp", "includeutils.h",
        "indexitem.cpp", "indexitem.h",
        "insertionpointlocator.cpp", "insertionpointlocator.h",
        "searchsymbols.cpp", "searchsymbols.h",
        "semantichighlighter.cpp", "semantichighlighter.h",
        "stringtable.cpp", "stringtable.h",
        "symbolfinder.cpp", "symbolfinder.h",
        "symbolsfindfilter.cpp", "symbolsfindfilter.h",
        "typehierarchybuilder.cpp", "typehierarchybuilder.h",
    ]

    Group {
        name: "Tests"
        condition: project.testsEnabled
        files: [
            "cppcodegen_test.cpp",
            "cppcompletion_test.cpp",
            "cppheadersource_test.cpp",
            "cpplocalsymbols_test.cpp",
            "cpplocatorfilter_test.cpp",
            "cppmodelmanager_test.cpp",
            "cpppointerdeclarationformatter_test.cpp",
            "cppsourceprocessertesthelper.cpp", "cppsourceprocessertesthelper.h",
            "cppsourceprocessor_test.cpp",
            "cpptoolstestcase.cpp", "cpptoolstestcase.h",
            "modelmanagertesthelper.cpp", "modelmanagertesthelper.h",
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
