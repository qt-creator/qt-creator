import qbs 1.0

QtcPlugin {
    name: "ClearCase"

    pluginjson.replacements: ({"CLEARCASE_DISABLED_STR": (qbs.targetOS.contains("macos") ? "true": "false")})

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "VcsBase" }

    files: [
        "activityselector.cpp",
        "activityselector.h",
        "annotationhighlighter.cpp",
        "annotationhighlighter.h",
        "checkoutdialog.cpp",
        "checkoutdialog.h",
        "clearcaseconstants.h",
        "clearcaseeditor.cpp",
        "clearcaseeditor.h",
        "clearcaseplugin.cpp",
        "clearcaseplugin.h",
        "clearcasesettings.cpp",
        "clearcasesettings.h",
        "clearcasesubmiteditor.cpp",
        "clearcasesubmiteditor.h",
        "clearcasesubmiteditorwidget.cpp",
        "clearcasesubmiteditorwidget.h",
        "clearcasesync.cpp",
        "clearcasesync.h",
        "clearcasetr.h",
        "settingspage.cpp",
        "settingspage.h",
        "versionselector.cpp",
        "versionselector.h",
    ]
}

