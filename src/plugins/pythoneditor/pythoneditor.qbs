import qbs.base 1.0

import QtcPlugin

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
            "pythoneditorfactory.cpp", "pythoneditorfactory.h",
            "pythoneditorplugin.cpp", "pythoneditorplugin.h",
            "pythoneditorplugin.qrc",
            "pythoneditorwidget.cpp", "pythoneditorwidget.h",
        ]
    }

    Group {
        name: "Tools"
        prefix: "tools/"
        files: [
            "lexical/pythonformattoken.h",
            "lexical/pythonscanner.h", "lexical/pythonscanner.cpp",
            "lexical/sourcecodestream.h",
            "pythonhighlighter.h", "pythonhighlighter.cpp",
            "pythonhighlighterfactory.h", "pythonhighlighterfactory.cpp",
            "pythonindenter.cpp", "pythonindenter.h"
        ]
    }

    Group {
        name: "Wizard"
        prefix: "wizard/"
        files: [
            "pythonclassnamepage.cpp", "pythonclassnamepage.h",
            "pythonclasswizard.h", "pythonclasswizard.cpp",
            "pythonclasswizarddialog.h", "pythonclasswizarddialog.cpp",
            "pythonfilewizard.h", "pythonfilewizard.cpp",
            "pythonsourcegenerator.h", "pythonsourcegenerator.cpp"
        ]
    }
}

