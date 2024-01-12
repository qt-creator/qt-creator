import qbs 1.0

QtcPlugin {
    name: "CMakeProjectManager"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "CppEditor" }
    Depends { name: "QmlJS" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "QtSupport" }

    files: [
        "builddirparameters.cpp",
        "builddirparameters.h",
        "cmake_global.h",
        "cmakeabstractprocessstep.cpp",
        "cmakeabstractprocessstep.h",
        "cmakebuildconfiguration.cpp",
        "cmakebuildconfiguration.h",
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
        "cmakeformatter.cpp",
        "cmakeformatter.h",
        "cmakeinstallstep.cpp",
        "cmakeinstallstep.h",
        "cmakekitaspect.h",
        "cmakekitaspect.cpp",
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
        "cmakeprojectmanagertr.h",
        "cmakeprojectnodes.cpp",
        "cmakeprojectnodes.h",
        "cmakeprojectplugin.cpp",
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
        "presetsparser.cpp",
        "presetsparser.h",
        "presetsmacros.cpp",
        "presetsmacros.h",
        "projecttreehelper.cpp",
        "projecttreehelper.h"
    ]

    Group {
        name: "3rdparty"
        cpp.includePaths: base.concat("3rdparty/cmake")

        prefix: "3rdparty/"
        files: [
            "cmake/cmListFileCache.cxx",
            "cmake/cmListFileCache.h",
            "cmake/cmListFileLexer.cxx",
            "cmake/cmListFileLexer.h",
            "cmake/cmStandardLexer.h",
            "rstparser/rstparser.cc",
            "rstparser/rstparser.h"
        ]
    }
}
