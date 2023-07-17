import qbs

QtcPlugin {
    name: "Cppcheck"

    Depends { name: "Core" }
    Depends { name: "CppEditor" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }

    Depends { name: "Qt.widgets" }

    files: [
        "cppcheckconstants.h",
        "cppcheckdiagnostic.cpp",
        "cppcheckdiagnostic.h",
        "cppcheckdiagnosticmanager.h",
        "cppcheckdiagnosticsmodel.cpp",
        "cppcheckdiagnosticsmodel.h",
        "cppcheckdiagnosticview.cpp",
        "cppcheckdiagnosticview.h",
        "cppcheckmanualrundialog.cpp",
        "cppcheckmanualrundialog.h",
        "cppcheckplugin.cpp",
        "cppcheckplugin.h",
        "cppcheckrunner.cpp",
        "cppcheckrunner.h",
        "cppchecksettings.cpp",
        "cppchecksettings.h",
        "cppchecktextmark.cpp",
        "cppchecktextmark.h",
        "cppchecktextmarkmanager.cpp",
        "cppchecktextmarkmanager.h",
        "cppchecktool.cpp",
        "cppchecktool.h",
        "cppchecktr.h",
        "cppchecktrigger.cpp",
        "cppchecktrigger.h"
    ]
}
