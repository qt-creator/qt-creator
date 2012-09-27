import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "CVS"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "Find" }
    Depends { name: "VcsBase" }
    Depends { name: "Locator" }

    files: [
        "annotationhighlighter.cpp",
        "annotationhighlighter.h",
        "checkoutwizard.cpp",
        "checkoutwizard.h",
        "checkoutwizardpage.cpp",
        "checkoutwizardpage.h",
        "cvs.qrc",
        "cvsconstants.h",
        "cvscontrol.cpp",
        "cvscontrol.h",
        "cvseditor.cpp",
        "cvseditor.h",
        "cvsplugin.cpp",
        "cvsplugin.h",
        "cvssettings.cpp",
        "cvssettings.h",
        "cvssubmiteditor.cpp",
        "cvssubmiteditor.h",
        "cvsutils.cpp",
        "cvsutils.h",
        "settingspage.cpp",
        "settingspage.h",
        "settingspage.ui",
    ]
}
