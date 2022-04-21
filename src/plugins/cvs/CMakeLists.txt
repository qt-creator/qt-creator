add_qtc_plugin(CVS
  PLUGIN_CLASS CvsPlugin
  PLUGIN_DEPENDS Core TextEditor VcsBase
  SOURCES
    annotationhighlighter.cpp annotationhighlighter.h
    cvseditor.cpp cvseditor.h
    cvsplugin.cpp cvsplugin.h
    cvssettings.cpp cvssettings.h
    cvssubmiteditor.cpp cvssubmiteditor.h
    cvsutils.cpp cvsutils.h
)
