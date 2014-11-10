import qbs 1.0

QtcPlugin {
    name: "Subversion"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "VcsBase" }

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
        "subversionclient.cpp",
        "subversionclient.h",
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

