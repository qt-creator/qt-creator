add_qtc_plugin(Cppcheck
  DEPENDS Qt5::Widgets
  PLUGIN_DEPENDS Core Debugger CppTools ProjectExplorer TextEditor
  SOURCES
    cppcheckconstants.h
    cppcheckdiagnostic.cpp cppcheckdiagnostic.h
    cppcheckdiagnosticmanager.h
    cppcheckdiagnosticsmodel.cpp cppcheckdiagnosticsmodel.h
    cppcheckdiagnosticview.cpp cppcheckdiagnosticview.h
    cppcheckmanualrundialog.cpp cppcheckmanualrundialog.h
    cppcheckoptions.cpp cppcheckoptions.h
    cppcheckplugin.cpp cppcheckplugin.h
    cppcheckrunner.cpp cppcheckrunner.h
    cppchecktextmark.cpp cppchecktextmark.h
    cppchecktextmarkmanager.cpp cppchecktextmarkmanager.h
    cppchecktool.cpp cppchecktool.h
    cppchecktrigger.cpp cppchecktrigger.h
)
