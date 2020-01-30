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
        "bazaarcommitpanel.ui",
        "bazaarcommitwidget.cpp",
        "bazaarcommitwidget.h",
        "bazaareditor.cpp",
        "bazaareditor.h",
        "bazaarplugin.cpp",
        "bazaarplugin.h",
        "bazaarsettings.cpp",
        "bazaarsettings.h",
        "branchinfo.cpp",
        "branchinfo.h",
        "commiteditor.cpp",
        "commiteditor.h",
        "constants.h",
        "optionspage.cpp",
        "optionspage.h",
        "optionspage.ui",
        "pullorpushdialog.cpp",
        "pullorpushdialog.h",
        "pullorpushdialog.ui",
        "revertdialog.ui",
        "uncommitdialog.ui",
    ]
}

