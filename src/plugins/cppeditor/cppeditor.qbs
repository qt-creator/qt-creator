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
        "cppcompleteswitch.cpp",
        "cppcompleteswitch.h",
        "cppeditor.cpp",
        "cppeditor.h",
        "cppeditor.qrc",
        "cppeditor_global.h",
        "cppeditorconstants.h",
        "cppeditorenums.h",
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
        "cppinsertdecldef.cpp",
        "cppinsertdecldef.h",
        "cppinsertqtpropertymembers.cpp",
        "cppinsertqtpropertymembers.h",
        "cppoutline.cpp",
        "cppoutline.h",
        "cppplugin.cpp",
        "cppplugin.h",
        "cppquickfix.cpp",
        "cppquickfix.h",
        "cppquickfixassistant.cpp",
        "cppquickfixassistant.h",
        "cppquickfixes.cpp",
        "cppsnippetprovider.cpp",
        "cppsnippetprovider.h",
        "cpptypehierarchy.cpp",
        "cpptypehierarchy.h",
    ]

    Group {
        condition: Defaults.testsEnabled(qbs)
        files: [
            "cppquickfix_test.cpp"
        ]

        cpp.defines: outer.concat(['SRCDIR="' + FileInfo.path(filePath) + '"'])
    }
}
