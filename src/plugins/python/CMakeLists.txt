add_qtc_plugin(Python
  PLUGIN_DEPENDS Core LanguageClient ProjectExplorer TextEditor
  SOURCES
    python.qrc
    pythoneditor.cpp pythoneditor.h
    pythonconstants.h
    pythonplugin.cpp pythonplugin.h
    pythonformattoken.h
    pythonhighlighter.cpp pythonhighlighter.h
    pythonindenter.cpp pythonindenter.h
    pythonproject.cpp pythonproject.h
    pythonrunconfiguration.cpp pythonrunconfiguration.h
    pythonsettings.cpp pythonsettings.h
    pythonscanner.cpp pythonscanner.h
    pythonutils.cpp pythonutils.h
)
