import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Perforce"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "Find" }
    Depends { name: "VcsBase" }
    Depends { name: "Locator" }

    Depends { name: "cpp" }
    cpp.includePaths: [
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "perforceplugin.h",
        "perforcechecker.h",
        "settingspage.h",
        "perforceeditor.h",
        "changenumberdialog.h",
        "perforcesubmiteditor.h",
        "pendingchangesdialog.h",
        "perforceconstants.h",
        "perforceversioncontrol.h",
        "perforcesettings.h",
        "annotationhighlighter.h",
        "perforcesubmiteditorwidget.h",
        "perforceplugin.cpp",
        "perforcechecker.cpp",
        "settingspage.cpp",
        "perforceeditor.cpp",
        "changenumberdialog.cpp",
        "perforcesubmiteditor.cpp",
        "pendingchangesdialog.cpp",
        "perforceversioncontrol.cpp",
        "perforcesettings.cpp",
        "annotationhighlighter.cpp",
        "perforcesubmiteditorwidget.cpp",
        "settingspage.ui",
        "changenumberdialog.ui",
        "pendingchangesdialog.ui",
        "submitpanel.ui",
        "perforce.qrc"
    ]
}

