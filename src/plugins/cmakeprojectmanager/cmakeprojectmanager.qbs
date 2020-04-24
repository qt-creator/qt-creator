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
        "cmake_global.h",
        "cmakebuildconfiguration.cpp",
        "cmakebuildconfiguration.h",
        "cmakebuildsettingswidget.cpp",
        "cmakebuildsettingswidget.h",
        "cmakebuildstep.cpp",
        "cmakebuildstep.h",
        "cmakebuildsystem.cpp",
        "cmakebuildsystem.h",
        "cmakebuildtarget.h",
        "cmakeconfigitem.cpp",
        "cmakeconfigitem.h",
        "cmakeeditor.cpp",
        "cmakeeditor.h",
        "cmakefilecompletionassist.cpp",
        "cmakefilecompletionassist.h",
        "cmakekitinformation.h",
        "cmakekitinformation.cpp",
        "cmakelocatorfilter.cpp",
        "cmakelocatorfilter.h",
        "cmakeparser.cpp",
        "cmakeparser.h",
        "cmakeprocess.cpp",
        "cmakeprocess.h",
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
        "cmaketool.cpp",
        "cmaketool.h",
        "cmaketoolmanager.cpp",
        "cmaketoolmanager.h",
        "cmaketoolsettingsaccessor.cpp",
        "cmaketoolsettingsaccessor.h",
        "cmakesettingspage.h",
        "cmakesettingspage.cpp",
        "cmakeindenter.h",
        "cmakeindenter.cpp",
        "cmakeautocompleter.h",
        "cmakeautocompleter.cpp",
        "cmakespecificsettings.h",
        "cmakespecificsettings.cpp",
        "cmakespecificsettingspage.h",
        "cmakespecificsettingspage.cpp",
        "cmakespecificsettingspage.ui",
        "configmodel.cpp",
        "configmodel.h",
        "configmodelitemdelegate.cpp",
        "configmodelitemdelegate.h",
        "fileapidataextractor.cpp",
        "fileapidataextractor.h",
        "fileapiparser.cpp",
        "fileapiparser.h",
        "fileapireader.cpp",
        "fileapireader.h",
        "projecttreehelper.cpp",
        "projecttreehelper.h"
    ]
}
