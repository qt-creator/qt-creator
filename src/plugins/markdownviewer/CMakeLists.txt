add_qtc_plugin(MarkdownViewer
  PLUGIN_DEPENDS Core TextEditor
  SOURCES
    markdownbrowser.cpp markdownbrowser.h
    markdowndocument.cpp markdowndocument.h
    markdowneditor.cpp markdowneditor.h
    markdownviewer.cpp markdownviewer.h
    markdownviewerconstants.h
    markdownviewerfactory.cpp markdownviewerfactory.h
    markdownviewerplugin.cpp markdownviewerplugin.h
    markdownviewerwidget.cpp markdownviewerwidget.h
)
