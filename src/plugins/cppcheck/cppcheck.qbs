import qbs

QtcPlugin {
    name: "Cppcheck"

    Depends { name: "Core" }
    Depends { name: "CppTools" }
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
        "cppcheckoptions.cpp",
        "cppcheckoptions.h",
        "cppcheckplugin.cpp",
        "cppcheckplugin.h",
        "cppcheckrunner.cpp",
        "cppcheckrunner.h",
        "cppchecktextmark.cpp",
        "cppchecktextmark.h",
        "cppchecktextmarkmanager.cpp",
        "cppchecktextmarkmanager.h",
        "cppchecktool.cpp",
        "cppchecktool.h",
        "cppchecktrigger.cpp",
        "cppchecktrigger.h"
    ]
}
