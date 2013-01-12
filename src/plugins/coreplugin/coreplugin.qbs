import qbs.base 1.0
import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Core"

    Depends {
        name: "Qt"
        submodules: [
            "core", "widgets", "xml", "network", "script", "sql", "help", "printsupport"
        ]
    }
    Depends { name: "Utils" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "Aggregation" }
    Depends { name: "app_version_header" }

    cpp.includePaths: base.concat([
        "../..",
        "../../shared/scriptwrapper",
        "dialogs",
        "editormanager",
        "progressmanager",
        "scriptmanager",
        "actionmanager"
    ])

    cpp.dynamicLibraries: {
        if (qbs.targetOS == "windows") return [
            "ole32",
            "user32"
        ]
    }

    files: [
        "basefilewizard.cpp",
        "basefilewizard.h",
        "core.qrc",
        "core_global.h",
        "coreconstants.h",
        "coreplugin.cpp",
        "coreplugin.h",
        "designmode.cpp",
        "designmode.h",
        "documentmanager.cpp",
        "documentmanager.h",
        "editmode.cpp",
        "editmode.h",
        "editortoolbar.cpp",
        "editortoolbar.h",
        "externaltool.cpp",
        "externaltool.h",
        "externaltoolmanager.h",
        "fancyactionbar.cpp",
        "fancyactionbar.h",
        "fancyactionbar.qrc",
        "fancytabwidget.cpp",
        "fancytabwidget.h",
        "featureprovider.cpp",
        "featureprovider.h",
        "fileiconprovider.cpp",
        "fileiconprovider.h",
        "fileutils.cpp",
        "fileutils.h",
        "findplaceholder.cpp",
        "findplaceholder.h",
        "generalsettings.cpp",
        "generalsettings.h",
        "generalsettings.ui",
        "generatedfile.cpp",
        "generatedfile.h",
        "helpmanager.cpp",
        "helpmanager.h",
        "icontext.cpp",
        "icontext.h",
        "icore.cpp",
        "icore.h",
        "icorelistener.h",
        "id.cpp",
        "id.h",
        "idocument.cpp",
        "idocument.h",
        "idocumentfactory.h",
        "ifilewizardextension.h",
        "imode.cpp",
        "imode.h",
        "inavigationwidgetfactory.cpp",
        "inavigationwidgetfactory.h",
        "infobar.cpp",
        "infobar.h",
        "ioutputpane.h",
        "iversioncontrol.cpp",
        "iversioncontrol.h",
        "mainwindow.cpp",
        "mainwindow.h",
        "manhattanstyle.cpp",
        "manhattanstyle.h",
        "messagemanager.cpp",
        "messagemanager.h",
        "messageoutputwindow.cpp",
        "messageoutputwindow.h",
        "mimedatabase.cpp",
        "mimedatabase.h",
        "mimetypemagicdialog.cpp",
        "mimetypemagicdialog.h",
        "mimetypemagicdialog.ui",
        "mimetypesettings.cpp",
        "mimetypesettings.h",
        "mimetypesettingspage.ui",
        "minisplitter.cpp",
        "minisplitter.h",
        "modemanager.cpp",
        "modemanager.h",
        "navigationsubwidget.cpp",
        "navigationsubwidget.h",
        "navigationwidget.cpp",
        "navigationwidget.h",
        "outputpane.cpp",
        "outputpane.h",
        "outputpanemanager.cpp",
        "outputpanemanager.h",
        "outputwindow.cpp",
        "outputwindow.h",
        "plugindialog.cpp",
        "plugindialog.h",
        "removefiledialog.cpp",
        "removefiledialog.h",
        "removefiledialog.ui",
        "rightpane.cpp",
        "rightpane.h",
        "settingsdatabase.cpp",
        "settingsdatabase.h",
        "sidebar.cpp",
        "sidebar.h",
        "sidebarwidget.cpp",
        "sidebarwidget.h",
        "statusbarmanager.cpp",
        "statusbarmanager.h",
        "statusbarwidget.cpp",
        "statusbarwidget.h",
        "styleanimator.cpp",
        "styleanimator.h",
        "tabpositionindicator.cpp",
        "tabpositionindicator.h",
        "textdocument.cpp",
        "textdocument.h",
        "toolsettings.cpp",
        "toolsettings.h",
        "variablechooser.cpp",
        "variablechooser.h",
        "variablechooser.ui",
        "variablemanager.cpp",
        "variablemanager.h",
        "vcsmanager.cpp",
        "vcsmanager.h",
        "versiondialog.cpp",
        "versiondialog.h",
        "actionmanager/actioncontainer.cpp",
        "actionmanager/actioncontainer.h",
        "actionmanager/actioncontainer_p.h",
        "actionmanager/actionmanager.cpp",
        "actionmanager/actionmanager.h",
        "actionmanager/actionmanager_p.h",
        "actionmanager/command.cpp",
        "actionmanager/command.h",
        "actionmanager/command_p.h",
        "actionmanager/commandbutton.cpp",
        "actionmanager/commandbutton.h",
        "actionmanager/commandmappings.cpp",
        "actionmanager/commandmappings.h",
        "actionmanager/commandmappings.ui",
        "actionmanager/commandsfile.cpp",
        "actionmanager/commandsfile.h",
        "dialogs/externaltoolconfig.cpp",
        "dialogs/externaltoolconfig.h",
        "dialogs/externaltoolconfig.ui",
        "dialogs/ioptionspage.cpp",
        "dialogs/ioptionspage.h",
        "dialogs/iwizard.cpp",
        "dialogs/iwizard.h",
        "dialogs/newdialog.cpp",
        "dialogs/newdialog.h",
        "dialogs/newdialog.ui",
        "dialogs/openwithdialog.cpp",
        "dialogs/openwithdialog.h",
        "dialogs/openwithdialog.ui",
        "dialogs/promptoverwritedialog.cpp",
        "dialogs/promptoverwritedialog.h",
        "dialogs/saveitemsdialog.cpp",
        "dialogs/saveitemsdialog.h",
        "dialogs/saveitemsdialog.ui",
        "dialogs/settingsdialog.cpp",
        "dialogs/settingsdialog.h",
        "dialogs/shortcutsettings.cpp",
        "dialogs/shortcutsettings.h",
        "editormanager/BinFiles.mimetypes.xml",
        "editormanager/editormanager.cpp",
        "editormanager/editormanager.h",
        "editormanager/editorview.cpp",
        "editormanager/editorview.h",
        "editormanager/ieditor.cpp",
        "editormanager/ieditor.h",
        "editormanager/ieditorfactory.cpp",
        "editormanager/ieditorfactory.h",
        "editormanager/iexternaleditor.cpp",
        "editormanager/iexternaleditor.h",
        "editormanager/openeditorsmodel.cpp",
        "editormanager/openeditorsmodel.h",
        "editormanager/openeditorsview.cpp",
        "editormanager/openeditorsview.h",
        "editormanager/openeditorswindow.cpp",
        "editormanager/openeditorswindow.h",
        "editormanager/systemeditor.cpp",
        "editormanager/systemeditor.h",
        "progressmanager/futureprogress.cpp",
        "progressmanager/futureprogress.h",
        "progressmanager/progressbar.cpp",
        "progressmanager/progressbar.h",
        "progressmanager/progressmanager.cpp",
        "progressmanager/progressmanager.h",
        "progressmanager/progressmanager_p.h",
        "progressmanager/progressview.cpp",
        "progressmanager/progressview.h",
        "scriptmanager/metatypedeclarations.h",
        "scriptmanager/scriptmanager.cpp",
        "scriptmanager/scriptmanager.h",
        "scriptmanager/scriptmanager_p.h",
    ]

    Group {
        condition: qbs.targetOS == "windows"
        files: [
            "progressmanager/progressmanager_win.cpp",
        ]
    }

    Group {
        condition: qbs.targetOS == "macx"
        files: [
            "progressmanager/progressmanager_mac.mm",
        ]
    }

    Group {
        condition: qbs.targetOS == "linux"
        files: [
            "progressmanager/progressmanager_x11.cpp",
        ]
    }

    ProductModule {
        Depends { name: "cpp" }
        Depends { name: "Aggregation" }
        Depends { name: "ExtensionSystem" }
        Depends { name: "Utils" }
    }
}
