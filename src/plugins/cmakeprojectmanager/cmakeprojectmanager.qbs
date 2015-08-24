import qbs 1.0

QtcPlugin {
    name: "CMakeProjectManager"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "QtSupport" }

    pluginRecommends: [
        "Designer"
    ]

    files: [
        "cmake_global.h",
        "cmakebuildconfiguration.cpp",
        "cmakebuildconfiguration.h",
        "cmakebuildinfo.h",
        "cmakebuildsettingswidget.cpp",
        "cmakebuildsettingswidget.h",
        "cmakecbpparser.cpp",
        "cmakecbpparser.h",
        "cmakeeditor.cpp",
        "cmakeeditor.h",
        "cmakefile.cpp",
        "cmakefile.h",
        "cmakefilecompletionassist.cpp",
        "cmakefilecompletionassist.h",
        "cmakekitconfigwidget.h",
        "cmakekitconfigwidget.cpp",
        "cmakekitinformation.h",
        "cmakekitinformation.cpp",
        "cmakelocatorfilter.cpp",
        "cmakelocatorfilter.h",
        "cmakeopenprojectwizard.cpp",
        "cmakeopenprojectwizard.h",
        "cmakeparser.cpp",
        "cmakeparser.h",
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
        "cmaketool.cpp",
        "cmaketool.h",
        "cmaketoolmanager.cpp",
        "cmaketoolmanager.h",
        "makestep.cpp",
        "makestep.h",
        "cmakesettingspage.h",
        "cmakesettingspage.cpp",
        "generatorinfo.h",
        "generatorinfo.cpp",
        "cmakeindenter.h",
        "cmakeindenter.cpp",
        "cmakeautocompleter.h",
        "cmakeautocompleter.cpp"
    ]
}
