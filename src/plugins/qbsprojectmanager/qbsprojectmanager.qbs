import qbs 1.0
import qbs.File
import qbs.FileInfo

QtcPlugin {
    name: "QbsProjectManager"
    type: base.concat(["qmltype-update"])

    Depends { name: "Qt"; submodules: [ "qml", "widgets" ] }

    Depends { name: "QmlJS" }
    Depends { name: "Utils" }

    Depends { name: "ProjectExplorer" }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "QtSupport" }
    Depends { name: "QmlJSTools" }
    Depends { name: "app_version_header" }

    files: [
        "customqbspropertiesdialog.h",
        "customqbspropertiesdialog.cpp",
        "customqbspropertiesdialog.ui",
        "defaultpropertyprovider.cpp",
        "defaultpropertyprovider.h",
        "propertyprovider.h",
        "qbsbuildconfiguration.cpp",
        "qbsbuildconfiguration.h",
        "qbsbuildstep.cpp",
        "qbsbuildstep.h",
        "qbscleanstep.cpp",
        "qbscleanstep.h",
        "qbscleanstepconfigwidget.ui",
        "qbsinstallstep.cpp",
        "qbsinstallstep.h",
        "qbskitinformation.cpp",
        "qbskitinformation.h",
        "qbsnodes.cpp",
        "qbsnodes.h",
        "qbsnodetreebuilder.cpp",
        "qbsnodetreebuilder.h",
        "qbspmlogging.cpp",
        "qbspmlogging.h",
        "qbsprofilemanager.cpp",
        "qbsprofilemanager.h",
        "qbsprofilessettingspage.cpp",
        "qbsprofilessettingspage.h",
        "qbsprofilessettingswidget.ui",
        "qbsproject.cpp",
        "qbsproject.h",
        "qbsprojectimporter.cpp",
        "qbsprojectimporter.h",
        "qbsprojectmanager.qrc",
        "qbsprojectmanager_global.h",
        "qbsprojectmanagerconstants.h",
        "qbsprojectmanagerplugin.cpp",
        "qbsprojectmanagerplugin.h",
        "qbsprojectparser.cpp",
        "qbsprojectparser.h",
        "qbssession.cpp",
        "qbssession.h",
        "qbssettings.cpp",
        "qbssettings.h",
    ]

    // QML typeinfo stuff
    Group {
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

    Depends { name: "qbs resources"; condition: project.qbsSubModuleExists }
    Rule {
        condition: project.qbsSubModuleExists
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

