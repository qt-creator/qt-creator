import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Qt4ProjectManager"

    Depends { name: "Qt"; submodules: ["widgets", "network"] }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "CppTools" }
    Depends { name: "Debugger" }
    Depends { name: "QmlJS" }
    Depends { name: "CPlusPlus" }
    Depends { name: "TextEditor" }
    Depends { name: "QmlJSTools" }

    pluginRecommends: [
        "Designer"
    ]

    Group {
        name: "General"
        files: [
            "addlibrarywizard.cpp", "addlibrarywizard.h",
            "buildconfigurationinfo.h",
            "externaleditors.cpp", "externaleditors.h",
            "findqt4profiles.cpp", "findqt4profiles.h",
            "librarydetailscontroller.cpp", "librarydetailscontroller.h",
            "librarydetailswidget.ui",
            "makestep.cpp", "makestep.h", "makestep.ui",
            "profilecompletionassist.cpp", "profilecompletionassist.h",
            "profileeditor.cpp", "profileeditor.h",
            "profileeditorfactory.cpp", "profileeditorfactory.h",
            "profilehighlighter.cpp", "profilehighlighter.h",
            "profilehighlighterfactory.cpp", "profilehighlighterfactory.h",
            "profilehoverhandler.cpp", "profilehoverhandler.h",
            "qmakebuildinfo.h",
            "qmakeparser.cpp", "qmakeparser.h",
            "qmakekitconfigwidget.cpp", "qmakekitconfigwidget.h",
            "qmakekitinformation.cpp", "qmakekitinformation.h",
            "qmakeparser.cpp", "qmakeparser.h",
            "qmakerunconfigurationfactory.cpp", "qmakerunconfigurationfactory.h",
            "qmakestep.cpp", "qmakestep.h", "qmakestep.ui",
            "qt4buildconfiguration.cpp", "qt4buildconfiguration.h",
            "qt4nodes.cpp", "qt4nodes.h",
            "qt4project.cpp", "qt4project.h",
            "qt4projectconfigwidget.cpp", "qt4projectconfigwidget.h", "qt4projectconfigwidget.ui",
            "qt4projectmanager.cpp", "qt4projectmanager.h",
            "qt4projectmanager.qrc",
            "qt4projectmanager_global.h",
            "qt4projectmanagerconstants.h",
            "qt4projectmanagerplugin.cpp", "qt4projectmanagerplugin.h",
            "qt4targetsetupwidget.cpp", "qt4targetsetupwidget.h",
            "qtmodulesinfo.cpp", "qtmodulesinfo.h",
            "winceqtversion.cpp", "winceqtversion.h",
            "winceqtversionfactory.cpp", "winceqtversionfactory.h"
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
        name: "Images"
        prefix: "images/"
        files: [
            "headers.png",
            "run_qmake.png",
            "run_qmake_small.png",
            "sources.png",
            "unknown.png",
        ]
    }

    Group {
        name: "Qt/Desktop"
        prefix: "qt-desktop/"
        files: [
            "desktopqtversion.cpp", "desktopqtversion.h",
            "desktopqtversionfactory.cpp", "desktopqtversionfactory.h",
            "qt4runconfiguration.cpp", "qt4runconfiguration.h",
            "simulatorqtversion.cpp", "simulatorqtversion.h",
            "simulatorqtversionfactory.cpp", "simulatorqtversionfactory.h"
        ]
    }

    Group {
        name: "Wizards"
        prefix: "wizards/"
        files: [
            "abstractmobileapp.cpp", "abstractmobileapp.h",
            "abstractmobileappwizard.cpp", "abstractmobileappwizard.h",
            "consoleappwizard.cpp", "consoleappwizard.h",
            "consoleappwizarddialog.cpp", "consoleappwizarddialog.h",
            "emptyprojectwizard.cpp", "emptyprojectwizard.h",
            "emptyprojectwizarddialog.cpp", "emptyprojectwizarddialog.h",
            "filespage.cpp", "filespage.h",
            "guiappwizard.cpp", "guiappwizard.h",
            "guiappwizarddialog.cpp", "guiappwizarddialog.h",
            "html5app.cpp", "html5app.h",
            "html5appwizard.cpp", "html5appwizard.h",
            "html5appwizardpages.cpp", "html5appwizardpages.h",
            "html5appwizardsourcespage.ui",
            "libraryparameters.cpp", "libraryparameters.h",
            "librarywizard.cpp", "librarywizard.h",
            "librarywizarddialog.cpp", "librarywizarddialog.h",
            "mobileapp.cpp", "mobileapp.h",
            "mobileappwizardgenericoptionspage.ui",
            "mobileappwizardharmattanoptionspage.ui",
            "mobileappwizardmaemooptionspage.ui",
            "mobileappwizardpages.cpp", "mobileappwizardpages.h",
            "mobilelibraryparameters.cpp", "mobilelibraryparameters.h",
            "mobilelibrarywizardoptionpage.cpp", "mobilelibrarywizardoptionpage.h", "mobilelibrarywizardoptionpage.ui",
            "modulespage.cpp", "modulespage.h",
            "qtprojectparameters.cpp", "qtprojectparameters.h",
            "qtquickapp.cpp", "qtquickapp.h",
            "qtquickappwizard.cpp", "qtquickappwizard.h",
            "qtquickappwizardpages.cpp", "qtquickappwizardpages.h",
            "qtquickcomponentsetoptionspage.ui",
            "qtwizard.cpp", "qtwizard.h",
            "subdirsprojectwizard.cpp", "subdirsprojectwizard.h",
            "subdirsprojectwizarddialog.cpp", "subdirsprojectwizarddialog.h",
            "testwizard.cpp", "testwizard.h",
            "testwizarddialog.cpp", "testwizarddialog.h",
            "testwizardpage.cpp", "testwizardpage.h",
            "testwizardpage.ui",
            "wizards.qrc"
        ]
    }

    Group {
        name: "Wizard Images"
        prefix: "wizards/images/"
        files: [
            "console.png",
            "gui.png",
            "html5app.png",
            "lib.png",
            "qtquickapp.png",
        ]
    }
}
