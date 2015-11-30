import qbs 1.0
import qbs.FileInfo

QtcPlugin {
    name: "QbsProjectManager"

    property var externalQbsIncludes: project.useExternalQbs
            ? [project.qbs_install_dir + "/include/qbs"] : []
    property var externalQbsLibraryPaths: project.useExternalQbs
            ? [project.qbs_install_dir + '/' + project.libDirName] : []
    property var externalQbsDynamicLibraries: {
        var libs = []
        if (!project.useExternalQbs)
            return libs;
        var suffix = "";
        if (qbs.targetOS.contains("windows")) {
            libs.push("shell32")
            if (qbs.enableDebugCode)
                suffix = "d";
        }
        libs.push("qbscore" + suffix, "qbsqtprofilesetup" + suffix);
        return libs
    }

    condition: project.buildQbsProjectManager

    property bool useInternalQbsProducts: project.qbsSubModuleExists && !project.useExternalQbs

    Depends { name: "Qt"; submodules: [ "widgets", "script" ] }
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
    Depends { name: "ResourceEditor" }
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
    cpp.rpaths: base.concat(externalQbsLibraryPaths)
    cpp.dynamicLibraries: base.concat(externalQbsDynamicLibraries)

    files: [
        "customqbspropertiesdialog.h",
        "customqbspropertiesdialog.cpp",
        "customqbspropertiesdialog.ui",
        "defaultpropertyprovider.cpp",
        "defaultpropertyprovider.h",
        "propertyprovider.h",
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
        "qbsconstants.h",
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
        "qbsprofilessettingspage.cpp",
        "qbsprofilessettingspage.h",
        "qbsprofilessettingswidget.ui",
        "qbsproject.cpp",
        "qbsproject.h",
        "qbsprojectfile.cpp",
        "qbsprojectfile.h",
        "qbsprojectmanager.cpp",
        "qbsprojectmanager.h",
        "qbsprojectmanager.qrc",
        "qbsprojectmanager_global.h",
        "qbsprojectmanagerconstants.h",
        "qbsprojectmanagerplugin.cpp",
        "qbsprojectmanagerplugin.h",
        "qbsprojectparser.cpp",
        "qbsprojectparser.h",
        "qbsrunconfiguration.cpp",
        "qbsrunconfiguration.h"
    ]
}

