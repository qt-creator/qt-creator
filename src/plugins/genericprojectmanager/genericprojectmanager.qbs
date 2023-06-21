import qbs 1.0
import qbs.FileInfo

QtcPlugin {
    name: "GenericProjectManager"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }

    pluginRecommends: [
        "CppEditor"
    ]

    pluginTestDepends: [
        "CppEditor",
    ]

    files: [
        "filesselectionwizardpage.cpp",
        "filesselectionwizardpage.h",
        "genericbuildconfiguration.cpp",
        "genericbuildconfiguration.h",
        "genericmakestep.cpp",
        "genericmakestep.h",
        "genericproject.cpp",
        "genericproject.h",
        "genericprojectconstants.h",
        "genericprojectfileseditor.cpp",
        "genericprojectfileseditor.h",
        "genericprojectmanagertr.h",
        "genericprojectplugin.cpp",
        "genericprojectplugin.h",
        "genericprojectwizard.cpp",
        "genericprojectwizard.h",
    ]
}
