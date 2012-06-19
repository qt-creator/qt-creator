import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "CppTools"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "Find" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Locator" }
    Depends { name: "CPlusPlus" }
    Depends { name: "LanguageUtils" }

    Depends { name: "cpp" }
    cpp.defines: base.concat(["QT_NO_CAST_TO_ASCII"])
    cpp.includePaths: [
        ".",
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "abstracteditorsupport.cpp",
        "abstracteditorsupport.h",
        "commentssettings.cpp",
        "commentssettings.h",
        "completionsettingspage.cpp",
        "completionsettingspage.h",
        "completionsettingspage.ui",
        "cppclassesfilter.cpp",
        "cppclassesfilter.h",
        "cppcodeformatter.cpp",
        "cppcodeformatter.h",
        "cppcodestylepreferences.cpp",
        "cppcodestylepreferences.h",
        "cppcodestylepreferencesfactory.cpp",
        "cppcodestylesettings.cpp",
        "cppcodestylesettings.h",
        "cppcodestylesettingspage.cpp",
        "cppcodestylesettingspage.h",
        "cppcodestylesettingspage.ui",
        "cppcompletionassist.cpp",
        "cppcompletionassist.h",
        "cppcurrentdocumentfilter.cpp",
        "cppcurrentdocumentfilter.h",
        "cppdoxygen.cpp",
        "cppdoxygen.h",
        "cppcompletionsupport.cpp",
        "cppcompletionsupport.h",
        "cppfilesettingspage.cpp",
        "cppfilesettingspage.h",
        "cppfilesettingspage.ui",
        "cppfindreferences.cpp",
        "cppfindreferences.h",
        "cppfunctionsfilter.cpp",
        "cppfunctionsfilter.h",
        "cpphighlightingsupportinternal.cpp",
        "cpphighlightingsupportinternal.h",
        "cpphighlightingsupport.cpp",
        "cpphighlightingsupport.h",
        "cpplocatorfilter.cpp",
        "cpplocatorfilter.h",
        "cppmodelmanager.cpp",
        "cppmodelmanager.h",
        "cppqtstyleindenter.cpp",
        "cppqtstyleindenter.h",
        "cpprefactoringchanges.cpp",
        "cpprefactoringchanges.h",
        "cppsemanticinfo.cpp",
        "cppsemanticinfo.h",
        "cppchecksymbols.cpp",
        "cppchecksymbols.h",
        "cpplocalsymbols.cpp",
        "cpplocalsymbols.h",
        "cpptools_global.h",
        "cpptoolsconstants.h",
        "cpptoolseditorsupport.cpp",
        "cpptoolseditorsupport.h",
        "cpptoolsplugin.cpp",
        "cpptoolsplugin.h",
        "cpptoolsreuse.cpp",
        "cpptoolsreuse.h",
        "cpptoolssettings.cpp",
        "cpptoolssettings.h",
        "doxygengenerator.cpp",
        "doxygengenerator.h",
        "insertionpointlocator.cpp",
        "insertionpointlocator.h",
        "searchsymbols.cpp",
        "searchsymbols.h",
        "symbolfinder.cpp",
        "symbolfinder.h",
        "symbolsfindfilter.cpp",
        "symbolsfindfilter.h",
        "uicodecompletionsupport.cpp",
        "uicodecompletionsupport.h",
        "cppcompletionassistprovider.cpp",
        "cppcompletionassistprovider.h",
        "cppcodestylepreferencesfactory.h",
        "ModelManagerInterface.cpp",
        "ModelManagerInterface.h",
        "TypeHierarchyBuilder.cpp",
        "TypeHierarchyBuilder.h"
    ]

    ProductModule {
        Depends { name: "CPlusPlus" }
    }
}

