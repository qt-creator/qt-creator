import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "CMakeProjectManager"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "CppTools" }
    Depends { name: "CPlusPlus" }
    Depends { name: "TextEditor" }
    Depends { name: "Locator" }
    Depends { name: "QtSupport" }

    Depends { name: "cpp" }
    cpp.includePaths: [
        ".",
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "CMakeProject.mimetypes.xml",
        "cmakebuildconfiguration.cpp",
        "cmakebuildconfiguration.h",
        "cmakeeditor.cpp",
        "cmakeeditor.h",
        "cmakeeditorfactory.cpp",
        "cmakeeditorfactory.h",
        "cmakehighlighter.cpp",
        "cmakehighlighter.h",
        "cmakelocatorfilter.cpp",
        "cmakelocatorfilter.h",
        "cmakeopenprojectwizard.cpp",
        "cmakeopenprojectwizard.h",
        "cmakeproject.cpp",
        "cmakeproject.h",
        "cmakeproject.qrc",
        "cmakeprojectconstants.h",
        "cmakeprojectmanager.cpp",
        "cmakeprojectmanager.h",
        "cmakeprojectnodes.cpp",
        "cmakeprojectnodes.h",
        "cmakeprojectplugin.cpp",
        "cmakeprojectplugin.h",
        "cmakerunconfiguration.cpp",
        "cmakerunconfiguration.h",
        "cmakeuicodemodelsupport.cpp",
        "cmakeuicodemodelsupport.h",
        "makestep.cpp",
        "makestep.h",
    ]
}

