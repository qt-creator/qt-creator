import qbs 1.0

QtcPlugin {
    name: "CMakeProjectManager"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "QmlJS" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "QtSupport" }
    Depends { name: "app_version_header" }

    pluginRecommends: [
        "Designer"
    ]

    files: [
        "builddirparameters.cpp",
        "builddirparameters.h",
        "builddirmanager.cpp",
        "builddirmanager.h",
        "builddirreader.cpp",
        "builddirreader.h",
        "cmake_global.h",
        "cmakebuildconfiguration.cpp",
        "cmakebuildconfiguration.h",
        "cmakebuildinfo.h",
        "cmakebuildsettingswidget.cpp",
        "cmakebuildsettingswidget.h",
        "cmakebuildstep.cpp",
        "cmakebuildstep.h",
        "cmakebuildtarget.cpp",
        "cmakebuildtarget.h",
        "cmakecbpparser.cpp",
        "cmakecbpparser.h",
        "cmakeconfigitem.cpp",
        "cmakeconfigitem.h",
        "cmakeeditor.cpp",
        "cmakeeditor.h",
        "cmakefilecompletionassist.cpp",
        "cmakefilecompletionassist.h",
        "cmakekitconfigwidget.h",
        "cmakekitconfigwidget.cpp",
        "cmakekitinformation.h",
        "cmakekitinformation.cpp",
        "cmakelocatorfilter.cpp",
        "cmakelocatorfilter.h",
        "cmakeparser.cpp",
        "cmakeparser.h",
        "cmakeproject.cpp",
        "cmakeproject.h",
        "cmakeproject.qrc",
        "cmakeprojectimporter.cpp",
        "cmakeprojectimporter.h",
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
        "cmakesettingspage.h",
        "cmakesettingspage.cpp",
        "cmakeindenter.h",
        "cmakeindenter.cpp",
        "cmakeautocompleter.h",
        "cmakeautocompleter.cpp",
        "configmodel.cpp",
        "configmodel.h",
        "configmodelitemdelegate.cpp",
        "configmodelitemdelegate.h",
        "servermode.cpp",
        "servermode.h",
        "servermodereader.cpp",
        "servermodereader.h",
        "tealeafreader.cpp",
        "tealeafreader.h",
        "treescanner.cpp",
        "treescanner.h"
    ]
}
