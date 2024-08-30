add_qtc_plugin(QmlJSEditor
  DEPENDS LanguageUtils QmlJS QmlEditorWidgets
  PLUGIN_DEPENDS Core ProjectExplorer QmlJSTools TextEditor LanguageClient
  SOURCES
    qmlexpressionundercursor.cpp qmlexpressionundercursor.h
    qmllsclient.h qmllsclient.cpp
    qmllssettings.h qmllssettings.cpp
    qmljsautocompleter.cpp qmljsautocompleter.h
    qmljscompletionassist.cpp qmljscompletionassist.h
    qmljscomponentfromobjectdef.cpp qmljscomponentfromobjectdef.h
    qmljscomponentnamedialog.cpp qmljscomponentnamedialog.h
    qmljseditingsettingspage.cpp qmljseditingsettingspage.h
    qmljseditor.cpp qmljseditor.h
    qmljseditor_global.h
    qmljseditortr.h
    qmljseditorconstants.h
    qmljseditordocument.cpp qmljseditordocument.h qmljseditordocument_p.h
    qmljseditorplugin.cpp qmljseditorplugin.h
    qmljsfindreferences.cpp qmljsfindreferences.h
    qmljshighlighter.cpp qmljshighlighter.h
    qmljshoverhandler.cpp qmljshoverhandler.h
    qmljsoutline.cpp qmljsoutline.h
    qmljsoutlinetreeview.cpp qmljsoutlinetreeview.h
    qmljsquickfix.cpp qmljsquickfix.h
    qmljsquickfixassist.cpp qmljsquickfixassist.h
    qmljsquickfixes.cpp
    qmljsreuse.cpp qmljsreuse.h
    qmljssemantichighlighter.cpp qmljssemantichighlighter.h
    qmljssemanticinfoupdater.cpp qmljssemanticinfoupdater.h
    qmljstextmark.cpp qmljstextmark.h
    qmljswrapinloader.cpp qmljswrapinloader.h
    qmloutlinemodel.cpp qmloutlinemodel.h
    qmltaskmanager.cpp qmltaskmanager.h
    quicktoolbar.cpp quicktoolbar.h
  EXPLICIT_MOC qmljseditor.h
)
