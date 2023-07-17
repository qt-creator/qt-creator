add_qtc_plugin(Cppcheck
  DEPENDS Qt::Widgets
  PLUGIN_DEPENDS Core Debugger CppEditor ProjectExplorer TextEditor
  SOURCES
    cppcheckconstants.h
    cppcheckdiagnostic.cpp cppcheckdiagnostic.h
    cppcheckdiagnosticmanager.h
    cppcheckdiagnosticsmodel.cpp cppcheckdiagnosticsmodel.h
    cppcheckdiagnosticview.cpp cppcheckdiagnosticview.h
    cppcheckmanualrundialog.cpp cppcheckmanualrundialog.h
    cppcheckplugin.cpp cppcheckplugin.h
    cppcheckrunner.cpp cppcheckrunner.h
    cppchecksettings.cpp cppchecksettings.h
    cppchecktextmark.cpp cppchecktextmark.h
    cppchecktextmarkmanager.cpp cppchecktextmarkmanager.h
    cppchecktool.cpp cppchecktool.h
    cppchecktr.h
    cppchecktrigger.cpp cppchecktrigger.h
)
