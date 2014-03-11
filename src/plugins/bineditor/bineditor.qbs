import qbs 1.0

import QtcPlugin

QtcPlugin {
    name: "BinEditor"

    Depends { name: "Qt.widgets" }
    Depends { name: "Aggregation" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }

    files: [
        "bineditor.cpp",
        "bineditor.h",
        "bineditorconstants.h",
        "bineditorplugin.cpp",
        "bineditorplugin.h",
        "markup.h",
    ]
}

