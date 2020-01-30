add_qtc_plugin(Subversion
  PLUGIN_DEPENDS Core DiffEditor TextEditor VcsBase
  SOURCES
    annotationhighlighter.cpp annotationhighlighter.h
    settingspage.cpp settingspage.h settingspage.ui
    subversionclient.cpp subversionclient.h
    subversionconstants.h
    subversioneditor.cpp subversioneditor.h
    subversionplugin.cpp subversionplugin.h
    subversionsettings.cpp subversionsettings.h
    subversionsubmiteditor.cpp subversionsubmiteditor.h
)
