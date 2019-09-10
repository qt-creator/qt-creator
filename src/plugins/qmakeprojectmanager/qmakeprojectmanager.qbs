import qbs 1.0

Project {
    name: "QmakeProjectManager"

    QtcDevHeaders { }

    QtcPlugin {
        Depends { name: "Qt"; submodules: ["widgets", "network"] }
        Depends { name: "QmlJS" }
        Depends { name: "Utils" }

        Depends { name: "Core" }
        Depends { name: "ProjectExplorer" }
        Depends { name: "QtSupport" }
        Depends { name: "CppTools" }
        Depends { name: "TextEditor" }
        Depends { name: "ResourceEditor" }
        Depends { name: "app_version_header" }

        pluginRecommends: [
            "Designer"
        ]

        Group {
            name: "General"
            files: [
                "addlibrarywizard.cpp", "addlibrarywizard.h",
                "externaleditors.cpp", "externaleditors.h",
                "librarydetailscontroller.cpp", "librarydetailscontroller.h",
                "librarydetailswidget.ui",
                "makefileparse.cpp", "makefileparse.h",
                "profilecompletionassist.cpp", "profilecompletionassist.h",
                "profileeditor.cpp", "profileeditor.h",
                "profilehighlighter.cpp", "profilehighlighter.h",
                "profilehoverhandler.cpp", "profilehoverhandler.h",
                "qmakebuildinfo.h",
                "qmakekitinformation.cpp", "qmakekitinformation.h",
                "qmakemakestep.cpp", "qmakemakestep.h",
                "qmakeparser.cpp", "qmakeparser.h",
                "qmakeparsernodes.cpp", "qmakeparsernodes.h",
                "qmakeprojectimporter.cpp", "qmakeprojectimporter.h",
                "qmakesettings.cpp", "qmakesettings.h",
                "qmakestep.cpp", "qmakestep.h", "qmakestep.ui",
                "qmakebuildconfiguration.cpp", "qmakebuildconfiguration.h",
                "qmakenodes.cpp", "qmakenodes.h",
                "qmakenodetreebuilder.cpp", "qmakenodetreebuilder.h",
                "qmakeproject.cpp", "qmakeproject.h",
                "qmakeprojectconfigwidget.cpp", "qmakeprojectconfigwidget.h",
                "qmakeprojectmanager.cpp", "qmakeprojectmanager.h",
                "qmakeprojectmanager.qrc",
                "qmakeprojectmanager_global.h",
                "qmakeprojectmanagerconstants.h",
                "qmakeprojectmanagerplugin.cpp", "qmakeprojectmanagerplugin.h",
            ]
        }

        Group {
            name: "Custom Widget Wizard"
            prefix: "customwidgetwizard/"
            files: [
                "classdefinition.cpp", "classdefinition.h", "classdefinition.ui",
                "classlist.cpp", "classlist.h",
                "customwidgetpluginwizardpage.cpp", "customwidgetpluginwizardpage.h", "customwidgetpluginwizardpage.ui",
                "customwidgetwidgetswizardpage.cpp", "customwidgetwidgetswizardpage.h", "customwidgetwidgetswizardpage.ui",
                "customwidgetwizard.cpp", "customwidgetwizard.h",
                "customwidgetwizarddialog.cpp", "customwidgetwizarddialog.h",
                "filenamingparameters.h",
                "plugingenerator.cpp", "plugingenerator.h",
                "pluginoptions.h"
            ]
        }

        Group {
            name: "Wizards"
            prefix: "wizards/"
            files: [
                "filespage.cpp", "filespage.h",
                "qtprojectparameters.cpp", "qtprojectparameters.h",
                "qtwizard.cpp", "qtwizard.h",
                "subdirsprojectwizard.cpp", "subdirsprojectwizard.h",
                "subdirsprojectwizarddialog.cpp", "subdirsprojectwizarddialog.h",
                "simpleprojectwizard.cpp", "simpleprojectwizard.h",
                "wizards.qrc"
            ]
        }

        Group {
            name: "Wizard Images"
            prefix: "wizards/images/"
            files: [
                "console.png",
                "gui.png",
                "lib.png",
                "qtquickapp.png",
            ]
        }

        Export {
            Depends { name: "QtSupport" }
        }
    }
}
