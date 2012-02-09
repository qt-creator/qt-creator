import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "CVS"

    Depends { name: "qt"; submodules: ['gui'] }
    Depends { name: "utils" }
    Depends { name: "extensionsystem" }
    Depends { name: "aggregation" }
    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "find" }
    Depends { name: "VCSBase" }
    Depends { name: "Locator" }

    Depends { name: "cpp" }
    cpp.includePaths: [
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "annotationhighlighter.h",
        "cvsplugin.h",
        "cvscontrol.h",
        "settingspage.h",
        "cvseditor.h",
        "cvssubmiteditor.h",
        "cvssettings.h",
        "cvsutils.h",
        "cvsconstants.h",
        "checkoutwizard.h",
        "checkoutwizardpage.h",
        "annotationhighlighter.cpp",
        "cvsplugin.cpp",
        "cvscontrol.cpp",
        "settingspage.cpp",
        "cvseditor.cpp",
        "cvssubmiteditor.cpp",
        "cvssettings.cpp",
        "cvsutils.cpp",
        "checkoutwizard.cpp",
        "checkoutwizardpage.cpp",
        "settingspage.ui",
        "cvs.qrc"
    ]
}
