import qbs 1.0

QtcPlugin {
    name: "QmlJSEditor"

    Depends { name: "Qt"; submodules: ["widgets"] }
    Depends { name: "LanguageUtils" }
    Depends { name: "Utils" }
    Depends { name: "QmlEditorWidgets" }
    Depends { name: "QmlJS" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QmlJSTools" }

    files: [
        "qmlexpressionundercursor.cpp",
        "qmlexpressionundercursor.h",
        "qmljsautocompleter.cpp",
        "qmljsautocompleter.h",
        "qmljscompletionassist.cpp",
        "qmljscompletionassist.h",
        "qmljscomponentfromobjectdef.cpp",
        "qmljscomponentfromobjectdef.h",
        "qmljscomponentnamedialog.cpp",
        "qmljscomponentnamedialog.h",
        "qmljscomponentnamedialog.ui",
        "qmljseditingsettingspage.cpp",
        "qmljseditingsettingspage.h",
        "qmljseditingsettingspage.ui",
        "qmljseditor.cpp",
        "qmljseditor.h",
        "qmljseditor_global.h",
        "qmljseditorconstants.h",
        "qmljseditordocument.cpp",
        "qmljseditordocument.h",
        "qmljseditordocument_p.h",
        "qmljseditorplugin.cpp",
        "qmljseditorplugin.h",
        "qmljsfindreferences.cpp",
        "qmljsfindreferences.h",
        "qmljshighlighter.cpp",
        "qmljshighlighter.h",
        "qmljshoverhandler.cpp",
        "qmljshoverhandler.h",
        "qmljsoutline.cpp",
        "qmljsoutline.h",
        "qmljsoutlinetreeview.cpp",
        "qmljsoutlinetreeview.h",
        "qmljspreviewrunner.cpp",
        "qmljspreviewrunner.h",
        "qmljsquickfix.cpp",
        "qmljsquickfix.h",
        "qmljsquickfixassist.cpp",
        "qmljsquickfixassist.h",
        "qmljsquickfixes.cpp",
        "qmljsreuse.cpp",
        "qmljsreuse.h",
        "qmljssemantichighlighter.cpp",
        "qmljssemantichighlighter.h",
        "qmljssemanticinfoupdater.cpp",
        "qmljssemanticinfoupdater.h",
        "qmljswrapinloader.cpp",
        "qmljswrapinloader.h",
        "qmloutlinemodel.cpp",
        "qmloutlinemodel.h",
        "qmltaskmanager.cpp",
        "qmltaskmanager.h",
        "quicktoolbar.cpp",
        "quicktoolbar.h",
    ]

    Export {
        Depends { name: "QmlJSTools" }
    }
}
