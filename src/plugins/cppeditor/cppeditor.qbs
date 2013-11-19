import qbs.base 1.0
import qbs.FileInfo

import QtcPlugin

QtcPlugin {
    name: "CppEditor"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "CPlusPlus" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }

    files: [
        "cppautocompleter.cpp",
        "cppautocompleter.h",
        "cppclasswizard.cpp",
        "cppclasswizard.h",
        "cppeditorconstants.h",
        "cppeditor.cpp",
        "cppeditorenums.h",
        "cppeditor_global.h",
        "cppeditor.h",
        "cppeditorplugin.cpp",
        "cppeditorplugin.h",
        "cppeditor.qrc",
        "cppelementevaluator.cpp",
        "cppelementevaluator.h",
        "cppfilewizard.cpp",
        "cppfilewizard.h",
        "cppfollowsymbolundercursor.cpp",
        "cppfollowsymbolundercursor.h",
        "cppfunctiondecldeflink.cpp",
        "cppfunctiondecldeflink.h",
        "cpphighlighter.cpp",
        "cpphighlighterfactory.cpp",
        "cpphighlighterfactory.h",
        "cpphighlighter.h",
        "cpphoverhandler.cpp",
        "cpphoverhandler.h",
        "cppincludehierarchy.cpp",
        "cppincludehierarchy.h",
        "cppincludehierarchyitem.cpp",
        "cppincludehierarchyitem.h",
        "cppincludehierarchymodel.cpp",
        "cppincludehierarchymodel.h",
        "cppincludehierarchytreeview.cpp",
        "cppincludehierarchytreeview.h",
        "cppoutline.cpp",
        "cppoutline.h",
        "cpppreprocessordialog.cpp",
        "cpppreprocessordialog.h",
        "cpppreprocessordialog.ui",
        "cppquickfixassistant.cpp",
        "cppquickfixassistant.h",
        "cppquickfix.cpp",
        "cppquickfixes.cpp",
        "cppquickfixes.h",
        "cppquickfix.h",
        "cppsnippetprovider.cpp",
        "cppsnippetprovider.h",
        "cpptypehierarchy.cpp",
        "cpptypehierarchy.h",
        "cppvirtualfunctionassistprovider.cpp",
        "cppvirtualfunctionassistprovider.h",
        "cppvirtualfunctionproposalitem.cpp",
        "cppvirtualfunctionproposalitem.h",
    ]

    Group {
        name: "Tests"
        condition: project.testsEnabled
        files: [
            "cppdoxygen_test.cpp",
            "cppquickfix_test.cpp",
            "cppquickfix_test_utils.cpp",
            "cppquickfix_test_utils.h",
            "fileandtokenactions_test.cpp",
            "followsymbol_switchmethoddecldef_test.cpp",
            "cppincludehierarchy_test.cpp",
        ]

        cpp.defines: outer.concat(['SRCDIR="' + FileInfo.path(filePath) + '"'])
    }
}
