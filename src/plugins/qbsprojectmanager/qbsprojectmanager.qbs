import qbs 1.0
import qbs.File
import qbs.FileInfo

QtcPlugin {
    name: "QbsProjectManager"
    type: base.concat(["qmltype-update"])

    property var externalQbsIncludes: project.useExternalQbs
            ? [project.qbs_install_dir + "/include/qbs"] : []
    property var externalQbsLibraryPaths: project.useExternalQbs
            ? [project.qbs_install_dir + '/' + qtc.libDirName] : []
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

    Depends { name: "Qt"; submodules: [ "widgets" ] }
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
    Depends { name: "app_version_header" }

    cpp.defines: base.concat([
        'QML_BUILD_STATIC_LIB',
        'QBS_ENABLE_PROJECT_FILE_UPDATES', // TODO: Take from installed qbscore module
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
        "qbsbuildinfo.cpp",
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
        "qbskitinformation.cpp",
        "qbskitinformation.h",
        "qbslogsink.cpp",
        "qbslogsink.h",
        "qbsnodes.cpp",
        "qbsnodes.h",
        "qbsnodetreebuilder.cpp",
        "qbsnodetreebuilder.h",
        "qbsparser.cpp",
        "qbsparser.h",
        "qbspmlogging.cpp",
        "qbspmlogging.h",
        "qbsprofilessettingspage.cpp",
        "qbsprofilessettingspage.h",
        "qbsprofilessettingswidget.ui",
        "qbsproject.cpp",
        "qbsproject.h",
        "qbsprojectimporter.cpp",
        "qbsprojectimporter.h",
        "qbsprojectmanager.cpp",
        "qbsprojectmanager.h",
        "qbsprojectmanager.qrc",
        "qbsprojectmanager_global.h",
        "qbsprojectmanagerconstants.h",
        "qbsprojectmanagerplugin.cpp",
        "qbsprojectmanagerplugin.h",
        "qbsprojectmanagersettings.cpp",
        "qbsprojectmanagersettings.h",
        "qbsprojectparser.cpp",
        "qbsprojectparser.h",
        "qbsrunconfiguration.cpp",
        "qbsrunconfiguration.h",
    ]

    // QML typeinfo stuff
    property bool updateQmlTypeInfo: useInternalQbsProducts
    Group {
        condition: !updateQmlTypeInfo
        name: "qbs qml type info"
        qbs.install: true
        qbs.installDir: FileInfo.joinPaths(qtc.ide_data_path, "qtcreator",
                                           "qml-type-descriptions")
        prefix: FileInfo.joinPaths(project.ide_source_tree, "share", "qtcreator",
                                   "qml-type-descriptions") + '/'
        files: [
            "qbs-bundle.json",
            "qbs.qmltypes",
        ]
    }

    Depends { name: "qbs resources" }
    Rule {
        condition: updateQmlTypeInfo
        inputsFromDependencies: ["qbs qml type descriptions", "qbs qml type bundle"]
        Artifact {
            filePath: "dummy." + input.fileName
            fileTags: ["qmltype-update"]
        }
        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "Updating " + input.fileName + " in Qt Creator repository";
            cmd.sourceCode = function() {
                var targetFilePath = FileInfo.joinPaths(project.ide_source_tree, "share",
                                                        "qtcreator", "qml-type-descriptions",
                                                        input.fileName);
                File.copy(input.filePath, targetFilePath);
            }
            return cmd;
        }
    }
}

