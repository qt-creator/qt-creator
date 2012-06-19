import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Subversion"

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
        "annotationhighlighter.h",
        "subversionplugin.h",
        "subversioncontrol.h",
        "settingspage.h",
        "subversioneditor.h",
        "subversionsubmiteditor.h",
        "subversionsettings.h",
        "checkoutwizard.h",
        "checkoutwizardpage.h",
        "subversionconstants.h",
        "annotationhighlighter.cpp",
        "subversionplugin.cpp",
        "subversioncontrol.cpp",
        "settingspage.cpp",
        "subversioneditor.cpp",
        "subversionsubmiteditor.cpp",
        "subversionsettings.cpp",
        "checkoutwizard.cpp",
        "checkoutwizardpage.cpp",
        "settingspage.ui",
        "subversion.qrc"
    ]
}

