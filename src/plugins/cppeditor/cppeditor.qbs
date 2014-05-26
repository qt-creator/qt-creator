import qbs 1.0
import qbs.FileInfo

import QtcPlugin

QtcPlugin {
    name: "CppEditor"

    Depends { name: "Qt.widgets" }
    Depends { name: "Aggregation" }
    Depends { name: "CPlusPlus" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }

    Depends { name: "app_version_header" }

    files: [
        "cppautocompleter.cpp", "cppautocompleter.h",
        "cppclasswizard.cpp", "cppclasswizard.h",
        "cppcodemodelinspectordialog.cpp", "cppcodemodelinspectordialog.h", "cppcodemodelinspectordialog.ui",
        "cppdocumentationcommenthelper.cpp", "cppdocumentationcommenthelper.h",
        "cppeditor.cpp", "cppeditor.h",
        "cppeditor.qrc",
        "cppeditor_global.h",
        "cppeditorconstants.h",
        "cppeditordocument.cpp", "cppeditordocument.h",
        "cppeditorenums.h",
        "cppeditoroutline.cpp", "cppeditoroutline.h",
        "cppeditorplugin.cpp", "cppeditorplugin.h",
        "cppelementevaluator.cpp", "cppelementevaluator.h",
        "cppfilewizard.cpp", "cppfilewizard.h",
        "cppfollowsymbolundercursor.cpp", "cppfollowsymbolundercursor.h",
        "cppfunctiondecldeflink.cpp", "cppfunctiondecldeflink.h",
        "cpphighlighter.cpp", "cpphighlighter.h",
        "cpphighlighterfactory.cpp", "cpphighlighterfactory.h",
        "cpphoverhandler.cpp", "cpphoverhandler.h",
        "cppincludehierarchy.cpp", "cppincludehierarchy.h",
        "cppincludehierarchyitem.cpp", "cppincludehierarchyitem.h",
        "cppincludehierarchymodel.cpp", "cppincludehierarchymodel.h",
        "cppincludehierarchytreeview.cpp", "cppincludehierarchytreeview.h",
        "cppinsertvirtualmethods.cpp",
        "cppinsertvirtualmethods.h",
        "cpplocalrenaming.cpp", "cpplocalrenaming.h",
        "cppoutline.cpp", "cppoutline.h",
        "cpppreprocessordialog.cpp", "cpppreprocessordialog.h", "cpppreprocessordialog.ui",
        "cppquickfix.cpp", "cppquickfix.h",
        "cppquickfixassistant.cpp", "cppquickfixassistant.h",
        "cppquickfixes.cpp", "cppquickfixes.h",
        "cppsnippetprovider.cpp", "cppsnippetprovider.h",
        "cpptypehierarchy.cpp", "cpptypehierarchy.h",
        "cppvirtualfunctionassistprovider.cpp", "cppvirtualfunctionassistprovider.h",
        "cppvirtualfunctionproposalitem.cpp", "cppvirtualfunctionproposalitem.h",
    ]

    Group {
        name: "Tests"
        condition: project.testsEnabled
        files: [
            "cppdoxygen_test.cpp",
            "cppeditortestcase.cpp", "cppeditortestcase.h",
            "cppincludehierarchy_test.cpp",
            "cppquickfix_test.cpp",
            "cppquickfix_test.h",
            "fileandtokenactions_test.cpp",
            "followsymbol_switchmethoddecldef_test.cpp",
        ]

        cpp.defines: outer.concat(['SRCDIR="' + FileInfo.path(filePath) + '"'])
    }
}
