add_qtc_plugin(Perforce
  PLUGIN_DEPENDS Core TextEditor VcsBase
  SOURCES
    annotationhighlighter.cpp annotationhighlighter.h
    changenumberdialog.cpp changenumberdialog.h
    pendingchangesdialog.cpp pendingchangesdialog.h
    perforcechecker.cpp perforcechecker.h
    perforceeditor.cpp perforceeditor.h
    perforceplugin.cpp perforceplugin.h
    perforcesettings.cpp perforcesettings.h
    perforcesubmiteditor.cpp perforcesubmiteditor.h
    perforcesubmiteditorwidget.cpp perforcesubmiteditorwidget.h
    perforcetr.h
)
