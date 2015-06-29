import qbs 1.0

QtcPlugin {
    name: "DiffEditor"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }

    pluginRecommends: [
        "CodePaster"
    ]

    files: [
        "diffeditor.cpp",
        "diffeditor.h",
        "diffeditor.qrc",
        "diffeditor_global.h",
        "diffeditorconstants.h",
        "diffeditorcontroller.cpp",
        "diffeditorcontroller.h",
        "diffeditordocument.cpp",
        "diffeditordocument.h",
        "diffeditorfactory.cpp",
        "diffeditorfactory.h",
        "diffeditorplugin.cpp",
        "diffeditorplugin.h",
        "differ.cpp",
        "differ.h",
        "diffutils.cpp",
        "diffutils.h",
        "diffview.cpp",
        "diffview.h",
        "selectabletexteditorwidget.cpp",
        "selectabletexteditorwidget.h",
        "sidebysidediffeditorwidget.cpp",
        "sidebysidediffeditorwidget.h",
        "unifieddiffeditorwidget.cpp",
        "unifieddiffeditorwidget.h",
    ]
}

