import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "BinEditor"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "Find" }

    Depends { name: "cpp" }
    cpp.defines: base.concat(["QT_NO_CAST_FROM_ASCII"])

    files: [
        "bineditor.cpp",
        "bineditor.h",
        "bineditorconstants.h",
        "bineditorplugin.cpp",
        "bineditorplugin.h",
        "markup.h",
    ]
}

