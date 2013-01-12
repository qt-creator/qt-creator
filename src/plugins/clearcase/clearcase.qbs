import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "ClearCase"
//    provider: "AudioCodes"

    condition: qbs.targetOS != "mac"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Find" }
    Depends { name: "VcsBase" }
    Depends { name: "Locator" }

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

