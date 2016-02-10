import qbs 1.0

QtcPlugin {
    name: "Perforce"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "VcsBase" }

    files: [
        "annotationhighlighter.cpp",
        "annotationhighlighter.h",
        "changenumberdialog.cpp",
        "changenumberdialog.h",
        "changenumberdialog.ui",
        "pendingchangesdialog.cpp",
        "pendingchangesdialog.h",
        "pendingchangesdialog.ui",
        "perforce.qrc",
        "perforcechecker.cpp",
        "perforcechecker.h",
        "perforceeditor.cpp",
        "perforceeditor.h",
        "perforceplugin.cpp",
        "perforceplugin.h",
        "perforcesettings.cpp",
        "perforcesettings.h",
        "perforcesubmiteditor.cpp",
        "perforcesubmiteditor.h",
        "perforcesubmiteditorwidget.cpp",
        "perforcesubmiteditorwidget.h",
        "perforceversioncontrol.cpp",
        "perforceversioncontrol.h",
        "settingspage.cpp",
        "settingspage.h",
        "settingspage.ui",
        "submitpanel.ui",
    ]
}
