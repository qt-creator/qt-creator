add_qtc_plugin(Perforce
  PLUGIN_DEPENDS Core TextEditor VcsBase
  SOURCES
    annotationhighlighter.cpp annotationhighlighter.h
    changenumberdialog.cpp changenumberdialog.h changenumberdialog.ui
    pendingchangesdialog.cpp pendingchangesdialog.h pendingchangesdialog.ui
    perforcechecker.cpp perforcechecker.h
    perforceeditor.cpp perforceeditor.h
    perforceplugin.cpp perforceplugin.h
    perforcesettings.cpp perforcesettings.h
    perforcesubmiteditor.cpp perforcesubmiteditor.h
    perforcesubmiteditorwidget.cpp perforcesubmiteditorwidget.h
    promptdialog.ui
    settingspage.cpp settingspage.h settingspage.ui
    submitpanel.ui
)
