add_qtc_plugin(ClearCase
  PLUGIN_DEPENDS Core ProjectExplorer TextEditor VcsBase
  SOURCES
    activityselector.cpp activityselector.h
    annotationhighlighter.cpp annotationhighlighter.h
    checkoutdialog.cpp checkoutdialog.h
    clearcaseconstants.h
    clearcaseeditor.cpp clearcaseeditor.h
    clearcaseplugin.cpp clearcaseplugin.h
    clearcasesettings.cpp clearcasesettings.h
    clearcasesubmiteditor.cpp clearcasesubmiteditor.h
    clearcasesubmiteditorwidget.cpp clearcasesubmiteditorwidget.h
    clearcasesync.cpp clearcasesync.h
    clearcasetr.h
    settingspage.cpp settingspage.h
    versionselector.cpp versionselector.h
)
