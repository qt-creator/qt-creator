import qbs 1.0
import qbs.FileInfo

import QtcPlugin

QtcPlugin {
    name: "QbsProjectManager"

    property var externalQbsIncludes: project.useExternalQbs
            ? [project.qbs_install_dir + "/include/qbs"] : []
    property var externalQbsLibraryPaths: project.useExternalQbs
            ? [project.qbs_install_dir + "/lib"] : []
    property var externalQbsRPaths: project.useExternalQbs
            ? [project.qbs_install_dir + "/lib"] : []
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

    condition: project.buildQbsProjectManager

    property bool useInternalQbsProducts: project.qbsSubModuleExists && !project.useExternalQbs

    Depends { name: "Qt"; submodules: [ "widgets", "script" ] }
    Depends { name: "Aggregation" }
    Depends {
        name: "qbscore"
        condition: product.useInternalQbsProducts
    }
    Depends {
        name: "qbsqtprofilesetup"
        condition: product.useInternalQbsProducts
    }
    Depends { name: "QmlJS" }
    Depends { name: "Utils" }

    Depends { name: "ProjectExplorer" }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "QtSupport" }
    Depends { name: "QmlJSTools" }

    cpp.defines: base.concat([
        'QML_BUILD_STATIC_LIB',
        'QBS_INSTALL_DIR="'
                + (project.useExternalQbs
                       ? FileInfo.fromWindowsSeparators(project.qbs_install_dir)
                       : '')
                + '"'
    ])
    cpp.includePaths: base.concat(externalQbsIncludes)
    cpp.libraryPaths: base.concat(externalQbsLibraryPaths)
    cpp.rpaths: base.concat(externalQbsRPaths)
    cpp.dynamicLibraries: base.concat(externalQbsDynamicLibraries)

    files: [
        "qbsprojectmanager.qrc",
        "defaultpropertyprovider.cpp",
        "defaultpropertyprovider.h",
        "propertyprovider.h",
        "qbsbuildconfiguration.cpp",
        "qbsbuildconfiguration.h",
        "qbsbuildconfigurationwidget.cpp",
        "qbsbuildconfigurationwidget.h",
        "qbsbuildinfo.h",
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
        "qbsrunconfiguration.h"
    ]
}

