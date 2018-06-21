import qbs 1.0

QtcPlugin {
    name: "Subversion"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "DiffEditor" }
    Depends { name: "TextEditor" }
    Depends { name: "VcsBase" }

    files: [
        "annotationhighlighter.cpp",
        "annotationhighlighter.h",
        "settingspage.cpp",
        "settingspage.h",
        "settingspage.ui",
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

