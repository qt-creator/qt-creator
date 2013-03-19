import qbs.base 1.0
import qbs.fileinfo as FileInfo

import "../QtcPlugin.qbs" as QtcPlugin
import "../../../qbs/defaults.js" as Defaults

QtcPlugin {
    name: "CppEditor"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "CPlusPlus" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "cpp" }

    cpp.includePaths: base.concat("../../libs/3rdparty")

    files: [
        "CppEditor.mimetypes.xml",
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
        "cppfunctiondecldeflink.cpp",
        "cppfunctiondecldeflink.h",
        "cpphighlighter.cpp",
        "cpphighlighter.h",
        "cpphoverhandler.cpp",
        "cpphoverhandler.h",
        "cppoutline.cpp",
        "cppoutline.h",
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
    ]

    Group {
        condition: Defaults.testsEnabled(qbs)
        files: [
            "cppdoxygen_test.cpp",
            "cppquickfix_test.cpp",
            "fileandtokenactions_test.cpp",
        ]

        cpp.defines: outer.concat(['SRCDIR="' + FileInfo.path(filePath) + '"'])
    }
}
