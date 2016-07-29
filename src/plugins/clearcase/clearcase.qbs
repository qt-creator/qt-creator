import qbs 1.0

QtcPlugin {
    name: "ClearCase"

    pluginJsonReplacements: ({"CLEARCASE_DISABLED_STR": (qbs.targetOS.contains("macos") ? "true": "false")})

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
        "checkoutdialog.ui",
        "clearcase.qrc",
        "clearcaseconstants.h",
        "clearcasecontrol.cpp",
        "clearcasecontrol.h",
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
        "settingspage.cpp",
        "settingspage.h",
        "settingspage.ui",
        "undocheckout.ui",
        "versionselector.cpp",
        "versionselector.h",
        "versionselector.ui",
    ]
}

