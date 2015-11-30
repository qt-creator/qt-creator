import qbs.base 1.0

import QtcPlugin

QtcPlugin {
    name: "ModelEditor"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "CPlusPlus" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "ModelingLib" }

    cpp.includePaths: [
        "qmt",
        "qstringparser",
        "qtserialization",
    ]

    files: [
        "actionhandler.cpp",
        "actionhandler.h",
        "classviewcontroller.cpp",
        "classviewcontroller.h",
        "componentviewcontroller.cpp",
        "componentviewcontroller.h",
        "diagramsviewmanager.cpp",
        "diagramsviewmanager.h",
        "dragtool.cpp",
        "dragtool.h",
        "editordiagramview.cpp",
        "editordiagramview.h",
        "elementtasks.cpp",
        "elementtasks.h",
        "extdocumentcontroller.cpp",
        "extdocumentcontroller.h",
        "modeldocument.cpp",
        "modeldocument.h",
        "modeleditor_constants.h",
        "modeleditor.cpp",
        "modeleditorfactory.cpp",
        "modeleditorfactory.h",
        "modeleditor_file_wizard.cpp",
        "modeleditor_file_wizard.h",
        "modeleditor_global.h",
        "modeleditor.h",
        "modeleditor_plugin.cpp",
        "modeleditor_plugin.h",
        "modelindexer.cpp",
        "modelindexer.h",
        "modelsmanager.cpp",
        "modelsmanager.h",
        "openelementvisitor.cpp",
        "openelementvisitor.h",
        "pxnodecontroller.cpp",
        "pxnodecontroller.h",
        "pxnodeutilities.cpp",
        "pxnodeutilities.h",
        "resources/modeleditor.qrc",
        "settingscontroller.cpp",
        "settingscontroller.h",
        "uicontroller.cpp",
        "uicontroller.h",
    ]
}
