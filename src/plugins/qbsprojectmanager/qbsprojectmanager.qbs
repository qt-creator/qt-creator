import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "QbsProjectManager"

    property var externalQbsDefines: project.useExternalQbs
                                     ? ['QBS_BUILD_DIR="' + project.qbs_build_dir +'"'] : []
    property var externalQbsIncludes: project.useExternalQbs ? [project.qbs_source_dir + "/src/lib"] : []
    property var externalQbsLibraryPaths: project.useExternalQbs ? [project.qbs_build_dir + "/lib"] : []
    property var externalQbsRPaths: project.useExternalQbs ? [project.qbs_build_dir + "/lib"] : []
    property var externalQbsDynamicLibraries: {
        var libs = []
        if (!project.useExternalQbs)
            return libs;
        if (qbs.targetOS.contains("windows")) {
            libs.push("shell32")
            if (qbs.enableDebugCode)
                libs.push("qbscored")
            else
                libs.push("qbscore")
        } else {
            libs.push("qbscore")
        }
        return libs
    }

    condition: project.useExternalQbs || project.qbsSubModuleExists

    Depends { name: "Qt"; submodules: [ "widgets", "script" ] }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "TextEditor" }
    Depends { name: "QtSupport" }
    Depends { name: "QmlJS" }
    Depends { name: "QmlJSTools" }
    Depends {
        name: "qbscore"
        condition: project.qbsSubModuleExists && !project.useExternalQbs
    }

    cpp.defines: base.concat([
        'QML_BUILD_STATIC_LIB'
    ]).concat(externalQbsDefines)
    cpp.includePaths: base.concat(externalQbsIncludes)
    cpp.libraryPaths: base.concat(externalQbsLibraryPaths)
    cpp.rpaths: base.concat(externalQbsRPaths)
    cpp.dynamicLibraries: base.concat(externalQbsDynamicLibraries)

    files: [
        "qbsbuildconfiguration.cpp",
        "qbsbuildconfiguration.h",
        "qbsbuildconfigurationwidget.cpp",
        "qbsbuildconfigurationwidget.h",
        "qbsbuildstep.cpp",
        "qbsbuildstep.h",
        "qbsbuildstepconfigwidget.ui",
        "qbscleanstep.cpp",
        "qbscleanstep.h",
        "qbscleanstepconfigwidget.ui",
        "qbsdeployconfigurationfactory.cpp",
        "qbsdeployconfigurationfactory.h",
        "qbsinstallstep.cpp",
        "qbsinstallstep.h",
        "qbsinstallstepconfigwidget.ui",
        "qbslogsink.cpp",
        "qbslogsink.h",
        "qbsnodes.cpp",
        "qbsnodes.h",
        "qbsparser.cpp",
        "qbsparser.h",
        "qbsproject.cpp",
        "qbsproject.h",
        "qbsprojectfile.cpp",
        "qbsprojectfile.h",
        "qbsprojectmanager.cpp",
        "qbsprojectmanager.h",
        "qbsprojectmanager_global.h",
        "qbsprojectmanagerconstants.h",
        "qbsprojectmanagerplugin.cpp",
        "qbsprojectmanagerplugin.h",
        "qbspropertylineedit.cpp",
        "qbspropertylineedit.h",
        "qbsrunconfiguration.cpp",
        "qbsrunconfiguration.h",
        "qbsstep.cpp",
        "qbsstep.h"
    ]
}

