add_qtc_plugin(Python
  PLUGIN_DEPENDS Core QtSupport ProjectExplorer TextEditor
  SOURCES
    pythoneditor.cpp pythoneditor.h
    pythonconstants.h
    pythonplugin.cpp pythonplugin.h
    pythonformattoken.h
    pythonhighlighter.cpp pythonhighlighter.h
    pythonindenter.cpp pythonindenter.h
    pythonscanner.cpp pythonscanner.h
)
