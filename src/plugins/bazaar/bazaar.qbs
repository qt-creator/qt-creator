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


    files: [
        "annotationhighlighter.cpp",
        "annotationhighlighter.h",
        "bazaar.qrc",
        "bazaarclient.cpp",
        "bazaarclient.h",
        "bazaarcommitpanel.ui",
        "bazaarcommitwidget.cpp",
        "bazaarcommitwidget.h",
        "bazaarcontrol.cpp",
        "bazaarcontrol.h",
        "bazaareditor.cpp",
        "bazaareditor.h",
        "bazaarplugin.cpp",
        "bazaarplugin.h",
        "bazaarsettings.cpp",
        "bazaarsettings.h",
        "branchinfo.cpp",
        "branchinfo.h",
        "cloneoptionspanel.cpp",
        "cloneoptionspanel.h",
        "cloneoptionspanel.ui",
        "clonewizard.cpp",
        "clonewizard.h",
        "clonewizardpage.cpp",
        "clonewizardpage.h",
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
        "images/bazaar.png",
    ]
}

