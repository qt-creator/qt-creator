add_qtc_plugin(PythonEditor
  PLUGIN_DEPENDS Core QtSupport ProjectExplorer TextEditor
  SOURCES
    pythoneditor.cpp pythoneditor.h
    pythoneditorconstants.h
    pythoneditorplugin.cpp pythoneditorplugin.h
    pythonformattoken.h
    pythonhighlighter.cpp pythonhighlighter.h
    pythonindenter.cpp pythonindenter.h
    pythonscanner.cpp pythonscanner.h
)
