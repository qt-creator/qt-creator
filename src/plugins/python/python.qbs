import qbs 1.0

QtcPlugin {
    name: "Python"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }

    Group {
        name: "General"
        files: [
            "python.qrc",
            "pythoneditor.cpp", "pythoneditor.h",
            "pythonconstants.h",
            "pythonplugin.cpp", "pythonplugin.h",
            "pythonhighlighter.h", "pythonhighlighter.cpp",
            "pythonindenter.cpp", "pythonindenter.h",
            "pythonformattoken.h",
            "pythonscanner.h", "pythonscanner.cpp",
            "pythonsettings.cpp", "pythonsettings.h",
        ]
    }
}
