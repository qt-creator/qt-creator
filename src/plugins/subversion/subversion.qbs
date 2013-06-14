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

    files: [
        "annotationhighlighter.cpp",
        "annotationhighlighter.h",
        "checkoutwizard.cpp",
        "checkoutwizard.h",
        "checkoutwizardpage.cpp",
        "checkoutwizardpage.h",
        "settingspage.cpp",
        "settingspage.h",
        "settingspage.ui",
        "subversion.qrc",
        "subversionconstants.h",
        "subversioncontrol.cpp",
        "subversioncontrol.h",
        "subversioneditor.cpp",
        "subversioneditor.h",
        "subversionplugin.cpp",
        "subversionplugin.h",
        "subversionsettings.cpp",
        "subversionsettings.h",
        "subversionsubmiteditor.cpp",
        "subversionsubmiteditor.h",
    ]
}

