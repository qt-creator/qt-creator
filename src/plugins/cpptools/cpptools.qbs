import qbs 1.0
import qbs.FileInfo

import QtcPlugin

QtcPlugin {
    name: "CppTools"

    Depends { name: "Qt.widgets" }
    Depends { name: "Aggregation" }
    Depends { name: "CPlusPlus" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }

    cpp.defines: base
    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.defines: base.concat("_SCL_SECURE_NO_WARNINGS")
    }

    files: [
        "abstracteditorsupport.cpp", "abstracteditorsupport.h",
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
        "cppcompletionassistprovider.cpp", "cppcompletionassistprovider.h",
        "cppcurrentdocumentfilter.cpp", "cppcurrentdocumentfilter.h",
        "cppdoxygen.cpp", "cppdoxygen.h",
        "cppfilesettingspage.cpp", "cppfilesettingspage.h", "cppfilesettingspage.ui",
        "cppfindreferences.cpp", "cppfindreferences.h",
        "cppfunctionsfilter.cpp", "cppfunctionsfilter.h",
        "cpphighlightingsupport.cpp", "cpphighlightingsupport.h",
        "cpphighlightingsupportinternal.cpp", "cpphighlightingsupportinternal.h",
        "cppindexingsupport.cpp", "cppindexingsupport.h",
        "cpplocalsymbols.cpp", "cpplocalsymbols.h",
        "cpplocatordata.cpp", "cpplocatordata.h",
        "cpplocatorfilter.cpp", "cpplocatorfilter.h",
        "cppmodelmanager.cpp", "cppmodelmanager.h",
        "cppmodelmanagerinterface.cpp", "cppmodelmanagerinterface.h",
        "cppmodelmanagersupport.cpp", "cppmodelmanagersupport.h",
        "cppmodelmanagersupportinternal.cpp", "cppmodelmanagersupportinternal.h",
        "cpppointerdeclarationformatter.cpp", "cpppointerdeclarationformatter.h",
        "cpppreprocessor.cpp", "cpppreprocessor.h",
        "cppprojectfile.cpp", "cppprojectfile.h",
        "cppqtstyleindenter.cpp", "cppqtstyleindenter.h",
        "cpprefactoringchanges.cpp", "cpprefactoringchanges.h",
        "cppsemanticinfo.cpp", "cppsemanticinfo.h",
        "cppsnapshotupdater.cpp", "cppsnapshotupdater.h",
        "cpptools_global.h",
        "cpptoolsconstants.h",
        "cpptoolseditorsupport.cpp", "cpptoolseditorsupport.h",
        "cpptoolsplugin.cpp", "cpptoolsplugin.h",
        "cpptoolsreuse.cpp", "cpptoolsreuse.h",
        "cpptoolssettings.cpp", "cpptoolssettings.h",
        "doxygengenerator.cpp", "doxygengenerator.h",
        "functionutils.cpp", "functionutils.h",
        "includeutils.cpp", "includeutils.h",
        "insertionpointlocator.cpp", "insertionpointlocator.h",
        "searchsymbols.cpp", "searchsymbols.h",
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
            "cpplocatorfilter_test.cpp",
            "cppmodelmanager_test.cpp",
            "cpppointerdeclarationformatter_test.cpp",
            "cpppreprocessertesthelper.cpp", "cpppreprocessertesthelper.h",
            "cpppreprocessor_test.cpp",
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
