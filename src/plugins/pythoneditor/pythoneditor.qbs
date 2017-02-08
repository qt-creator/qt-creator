import qbs 1.0

QtcPlugin {
    name: "PythonEditor"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "QtSupport" }
    Depends { name: "ProjectExplorer" }

    Group {
        name: "General"
        files: [
            "pythoneditor.cpp", "pythoneditor.h",
            "pythoneditorconstants.h",
            "pythoneditorplugin.cpp", "pythoneditorplugin.h",
            "pythonhighlighter.h", "pythonhighlighter.cpp",
            "pythonindenter.cpp", "pythonindenter.h",
            "pythonformattoken.h",
            "pythonscanner.h", "pythonscanner.cpp",
        ]
    }
}
