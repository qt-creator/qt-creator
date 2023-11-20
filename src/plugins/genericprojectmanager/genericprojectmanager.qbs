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
        "genericprojectwizard.cpp",
        "genericprojectwizard.h",
    ]
}
