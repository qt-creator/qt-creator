import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Bazaar"

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
        "bazaar.qrc",
        "bazaarcommitpanel.ui",
        "cloneoptionspanel.ui",
        "pullorpushdialog.ui",
        "revertdialog.ui",
        "bazaarcommitwidget.cpp",
        "bazaarcommitwidget.h",
        "bazaarsettings.cpp",
        "branchinfo.cpp",
        "branchinfo.h",
        "cloneoptionspanel.cpp",
        "cloneoptionspanel.h",
        "constants.h",
        "optionspage.ui",
        "pullorpushdialog.cpp",
        "pullorpushdialog.h",
        "annotationhighlighter.cpp",
        "annotationhighlighter.h",
        "bazaarclient.cpp",
        "bazaarclient.h",
        "bazaarcontrol.cpp",
        "bazaarcontrol.h",
        "bazaareditor.cpp",
        "bazaareditor.h",
        "bazaarplugin.cpp",
        "bazaarplugin.h",
        "bazaarsettings.h",
        "clonewizard.cpp",
        "clonewizard.h",
        "clonewizardpage.cpp",
        "clonewizardpage.h",
        "commiteditor.cpp",
        "commiteditor.h",
        "optionspage.cpp",
        "optionspage.h",
        "images/bazaar.png"
    ]
}

