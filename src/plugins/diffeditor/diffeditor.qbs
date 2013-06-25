import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "DiffEditor"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "Find" }


    files: [
        "diffeditor.cpp",
        "diffeditor.h",
        "diffeditor_global.h",
        "diffeditorconstants.h",
        "diffeditorfactory.cpp",
        "diffeditorfactory.h",
        "diffeditorfile.cpp",
        "diffeditorfile.h",
        "diffeditorplugin.cpp",
        "diffeditorplugin.h",
        "diffeditorwidget.cpp",
        "diffeditorwidget.h",
        "differ.cpp",
        "differ.h",
        "diffshoweditor.cpp",
        "diffshoweditor.h",
        "diffshoweditorfactory.cpp",
        "diffshoweditorfactory.h",
    ]
}

