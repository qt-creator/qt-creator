add_qtc_plugin(Python
  DEPENDS QmlJS
  PLUGIN_DEPENDS Core LanguageClient ProjectExplorer TextEditor QtSupport
  SOURCES
    pipsupport.cpp pipsupport.h
    pyside.cpp pyside.h
    pythonbuildconfiguration.cpp pythonbuildconfiguration.h
    pysideuicextracompiler.cpp pysideuicextracompiler.h
    python.qrc
    pythonbuildsystem.cpp pythonbuildsystem.h
    pythonconstants.h
    pythoneditor.cpp pythoneditor.h
    pythonformattoken.h
    pythonhighlighter.cpp pythonhighlighter.h
    pythonindenter.cpp pythonindenter.h
    pythonkitaspect.h pythonkitaspect.cpp
    pythonlanguageclient.cpp pythonlanguageclient.h
    pythonplugin.cpp
    pythonproject.cpp pythonproject.h
    pythonrunconfiguration.cpp pythonrunconfiguration.h
    pythonscanner.cpp pythonscanner.h
    pythonsettings.cpp pythonsettings.h
    pythontr.h
    pythonutils.cpp pythonutils.h
    pythonwizardpage.cpp pythonwizardpage.h
)
