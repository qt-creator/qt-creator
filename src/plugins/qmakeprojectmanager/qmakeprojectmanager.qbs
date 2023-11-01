QtcPlugin {
    name: "QmakeProjectManager"

    Depends { name: "Qt"; submodules: ["widgets", "network"] }
    Depends { name: "QmlJS" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "CppEditor" }
    Depends { name: "TextEditor" }
    Depends { name: "ResourceEditor" }

    Group {
        name: "General"
        files: [
            "addlibrarywizard.cpp", "addlibrarywizard.h",
            "librarydetailscontroller.cpp", "librarydetailscontroller.h",
            "makefileparse.cpp", "makefileparse.h",
            "profilecompletionassist.cpp", "profilecompletionassist.h",
            "profileeditor.cpp", "profileeditor.h",
            "profilehighlighter.cpp", "profilehighlighter.h",
            "profilehoverhandler.cpp", "profilehoverhandler.h",
            "qmakebuildinfo.h",
            "qmakekitaspect.cpp", "qmakekitaspect.h",
            "qmakemakestep.cpp", "qmakemakestep.h",
            "qmakeparser.cpp", "qmakeparser.h",
            "qmakeparsernodes.cpp", "qmakeparsernodes.h",
            "qmakeprojectimporter.cpp", "qmakeprojectimporter.h",
            "qmakesettings.cpp", "qmakesettings.h",
            "qmakestep.cpp", "qmakestep.h",
            "qmakebuildconfiguration.cpp", "qmakebuildconfiguration.h",
            "qmakenodes.cpp", "qmakenodes.h",
            "qmakenodetreebuilder.cpp", "qmakenodetreebuilder.h",
            "qmakeproject.cpp", "qmakeproject.h",
            "qmakeprojectmanager.qrc",
            "qmakeprojectmanager_global.h", "qmakeprojectmanagertr.h",
            "qmakeprojectmanagerconstants.h",
            "qmakeprojectmanagerplugin.cpp", "qmakeprojectmanagerplugin.h",
        ]
    }

    Group {
        name: "Custom Widget Wizard"
        prefix: "customwidgetwizard/"
        files: [
            "classdefinition.cpp", "classdefinition.h",
            "classlist.cpp", "classlist.h",
            "customwidgetpluginwizardpage.cpp", "customwidgetpluginwizardpage.h",
            "customwidgetwidgetswizardpage.cpp", "customwidgetwidgetswizardpage.h",
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
            "qtprojectparameters.cpp", "qtprojectparameters.h",
            "qtwizard.cpp", "qtwizard.h",
            "subdirsprojectwizard.cpp", "subdirsprojectwizard.h",
            "subdirsprojectwizarddialog.cpp", "subdirsprojectwizarddialog.h",
            "wizards.qrc"
        ]
    }

    Group {
        name: "Wizard Images"
        prefix: "wizards/images/"
        files: ["*.png"]
    }

    Export {
        Depends { name: "QtSupport" }
    }
}
