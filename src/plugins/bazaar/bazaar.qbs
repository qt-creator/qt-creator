import qbs 1.0

QtcPlugin {
    name: "Bazaar"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "VcsBase" }

    files: [
        "annotationhighlighter.cpp",
        "annotationhighlighter.h",
        "bazaarclient.cpp",
        "bazaarclient.h",
        "bazaarcommitwidget.cpp",
        "bazaarcommitwidget.h",
        "bazaareditor.cpp",
        "bazaareditor.h",
        "bazaarplugin.cpp",
        "bazaarplugin.h",
        "bazaarsettings.cpp",
        "bazaarsettings.h",
        "bazaartr.h",
        "branchinfo.cpp",
        "branchinfo.h",
        "commiteditor.cpp",
        "commiteditor.h",
        "constants.h",
        "pullorpushdialog.cpp",
        "pullorpushdialog.h",
    ]
}

