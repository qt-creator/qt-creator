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
        "diffeditor_global.h", "diffeditortr.h",
        "diffeditorconstants.h",
        "diffeditoricons.h",
        "diffeditorcontroller.cpp",
        "diffeditorcontroller.h",
        "diffeditordocument.cpp",
        "diffeditordocument.h",
        "diffeditorplugin.cpp",
        "diffeditorwidgetcontroller.cpp",
        "diffeditorwidgetcontroller.h",
        "diffenums.h",
        "diffutils.cpp",
        "diffutils.h",
        "inlinediff.cpp",
        "inlinediff.h",
        "inlinediff_p.h",
        "selectabletexteditorwidget.cpp",
        "selectabletexteditorwidget.h",
        "sidebysidediffeditorwidget.cpp",
        "sidebysidediffeditorwidget.h",
        "unifieddiffeditorwidget.cpp",
        "unifieddiffeditorwidget.h",
    ]

    Group {
        name: "images"
        fileTags: "qt.core.resource_data"
        files: [
            "images/sidebysidediff.png",
            "images/sidebysidediff@2x.png",
            "images/unifieddiff.png",
            "images/unifieddiff@2x.png",
            "images/topbar.png",
            "images/topbar@2x.png",
        ]
    }
}
