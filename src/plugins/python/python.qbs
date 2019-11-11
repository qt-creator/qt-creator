import qbs 1.0

QtcPlugin {
    name: "Python"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "LanguageClient" }
    Depends { name: "LanguageServerProtocol" }

    Group {
        name: "General"
        files: [
            "python.qrc",
            "pythoneditor.cpp",
            "pythoneditor.h",
            "pythonconstants.h",
            "pythonplugin.cpp",
            "pythonplugin.h",
            "pythonhighlighter.h",
            "pythonhighlighter.cpp",
            "pythonindenter.cpp",
            "pythonindenter.h",
            "pythonformattoken.h",
            "pythonproject.cpp",
            "pythonproject.h",
            "pythonrunconfiguration.cpp",
            "pythonrunconfiguration.h",
            "pythonscanner.h",
            "pythonscanner.cpp",
            "pythonsettings.cpp",
            "pythonsettings.h",
            "pythonutils.cpp",
            "pythonutils.h",
        ]
    }
}
