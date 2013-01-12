import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "BinEditor"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "Find" }

    files: [
        "bineditor.cpp",
        "bineditor.h",
        "bineditorconstants.h",
        "bineditorplugin.cpp",
        "bineditorplugin.h",
        "markup.h",
    ]
}

