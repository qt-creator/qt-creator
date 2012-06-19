import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "GenericProjectManager"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "CPlusPlus" }
    Depends { name: "CppTools" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Find" }
    Depends { name: "Locator" }
    Depends { name: "QtSupport" }

    Depends { name: "cpp" }
    cpp.includePaths: [
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "genericproject.h",
        "genericprojectplugin.h",
        "generictarget.h",
        "genericprojectmanager.h",
        "genericprojectconstants.h",
        "genericprojectnodes.h",
        "genericprojectwizard.h",
        "genericprojectfileseditor.h",
        "pkgconfigtool.h",
        "genericmakestep.h",
        "genericbuildconfiguration.h",
        "selectablefilesmodel.h",
        "filesselectionwizardpage.h",
        "genericproject.cpp",
        "genericprojectplugin.cpp",
        "generictarget.cpp",
        "genericprojectmanager.cpp",
        "genericprojectnodes.cpp",
        "genericprojectwizard.cpp",
        "genericprojectfileseditor.cpp",
        "pkgconfigtool.cpp",
        "genericmakestep.cpp",
        "genericbuildconfiguration.cpp",
        "selectablefilesmodel.cpp",
        "filesselectionwizardpage.cpp",
        "genericmakestep.ui",
        "genericproject.qrc",
    ]
}

