import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "PythonEditor"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "CppTools" }
    Depends { name: "cpp" }

    cpp.defines: base.concat(["PYEDITOR_LIBRARY"])

    files: [
        "pythoneditor.cpp",
        "pythoneditor.h",
        "pythoneditor.mimetypes.xml",
        "pythoneditor_global.h",
        "pythoneditorconstants.h",
        "pythoneditorfactory.cpp",
        "pythoneditorfactory.h",
        "pythoneditorfeatures.h",
        "pythoneditorplugin.cpp",
        "pythoneditorplugin.h",
        "pythoneditorplugin.qrc",
        "pythoneditorwidget.cpp",
        "pythoneditorwidget.h",
        "tools/lexical/sourcecodestream.h",
        "tools/lexical/pythonscanner.h",
        "tools/lexical/pythonformattoken.h",
        "tools/lexical/pythonscanner.cpp",
        "tools/pythonhighlighter.h",
        "tools/pythonindenter.cpp",
        "tools/pythonhighlighter.cpp",
        "tools/pythonindenter.h",
        "wizard/pythonfilewizard.h",
        "wizard/pythonfilewizard.cpp",
    ]
}

