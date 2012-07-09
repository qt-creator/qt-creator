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

    cpp.includePaths: [
        ".",
        "..",
        "../..",
        "../../libs",
        "../../../src/shared/scriptwrapper/",
        "dialogs",
        "editormanager",
        "progressmanager",
        "scriptmanager",
        "actionmanager",
         buildDirectory
    ]

    cpp.dynamicLibraries: {
        if (qbs.targetOS == "windows") return [
            "ole32"
        ]
    }

    files: [
        "fancyactionbar.qrc",
        "generalsettings.ui",
        "basefilewizard.cpp",
        "basefilewizard.h",
        "core.qrc",
        "core_global.h",
        "coreconstants.h",
        "coreplugin.cpp",
        "coreplugin.h",
        "designmode.cpp",
        "designmode.h",
        "editmode.cpp",
        "editmode.h",
        "editortoolbar.cpp",
        "editortoolbar.h",
        "externaltool.cpp",
        "externaltool.h",
        "externaltoolmanager.h",
        "fancyactionbar.cpp",
        "fancyactionbar.h",
        "fancytabwidget.cpp",
        "fancytabwidget.h",
        "featureprovider.h",
        "featureprovider.cpp",
        "fileiconprovider.cpp",
        "fileiconprovider.h",
        "fileutils.cpp",
        "fileutils.h",
        "findplaceholder.cpp",
        "findplaceholder.h",
        "generalsettings.cpp",
        "generalsettings.h",
        "generatedfile.cpp",
        "generatedfile.h",
        "helpmanager.cpp",
        "helpmanager.h",
        "icontext.cpp",
        "icontext.h",
        "icore.cpp",
        "icore.h",
        "icorelistener.h",
        "id.h",
        "ifilewizardextension.h",
        "imode.cpp",
        "imode.h",
        "documentmanager.cpp",
        "documentmanager.h",
        "idocument.cpp",
        "idocument.h",
        "idocumentfactory.h",
        "inavigationwidgetfactory.cpp",
        "inavigationwidgetfactory.h",
        "infobar.cpp",
        "infobar.h",
        "ioutputpane.h",
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
        "variablechooser.h",
        "variablechooser.ui",
        "vcsmanager.h",
        "versiondialog.cpp",
        "versiondialog.h",
        "id.cpp",
        "iversioncontrol.h",
        "variablechooser.cpp",
        "variablemanager.cpp",
        "variablemanager.h",
        "vcsmanager.cpp",
        "actionmanager/actioncontainer.cpp",
        "actionmanager/actioncontainer.h",
        "actionmanager/actioncontainer_p.h",
        "actionmanager/actionmanager.cpp",
        "actionmanager/actionmanager.h",
        "actionmanager/actionmanager_p.h",
        "actionmanager/command.cpp",
        "actionmanager/command.h",
        "actionmanager/command_p.h",
        "actionmanager/commandbutton.h",
        "actionmanager/commandbutton.cpp",
        "actionmanager/commandmappings.cpp",
        "actionmanager/commandmappings.h",
        "actionmanager/commandmappings.ui",
        "actionmanager/commandsfile.cpp",
        "actionmanager/commandsfile.h",
        "dialogs/externaltoolconfig.ui",
        "dialogs/newdialog.ui",
        "dialogs/externaltoolconfig.cpp",
        "dialogs/externaltoolconfig.h",
        "dialogs/ioptionspage.cpp",
        "dialogs/ioptionspage.h",
        "dialogs/iwizard.cpp",
        "dialogs/iwizard.h",
        "dialogs/newdialog.cpp",
        "dialogs/newdialog.h",
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
        "editormanager/editorview.cpp",
        "editormanager/editorview.h",
        "editormanager/ieditor.cpp",
        "editormanager/ieditor.h",
        "editormanager/ieditorfactory.h",
        "editormanager/ieditorfactory.cpp",
        "editormanager/iexternaleditor.cpp",
        "editormanager/iexternaleditor.h",
        "editormanager/openeditorsmodel.cpp",
        "editormanager/openeditorsmodel.h",
        "editormanager/openeditorsview.cpp",
        "editormanager/openeditorsview.h",
        "editormanager/openeditorsview.ui",
        "editormanager/openeditorswindow.cpp",
        "editormanager/openeditorswindow.h",
        "editormanager/systemeditor.cpp",
        "editormanager/systemeditor.h",
        "editormanager/editormanager.cpp",
        "editormanager/editormanager.h",
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
        "scriptmanager/scriptmanager_p.h"
    ]

    Group {
        condition: qbs.targetOS == "windows"
        files: [
            "progressmanager/progressmanager_win.cpp"
        ]
    }

    Group {
        condition: qbs.targetOS == "macx"
        files: [
            "progressmanager/progressmanager_mac.mm"
        ]
    }

    Group {
        condition: qbs.targetOS == "linux"
        files: [
            "progressmanager/progressmanager_x11.cpp"
        ]
    }

    ProductModule {
        Depends { name: "cpp" }
        Depends { name: "Aggregation" }
        Depends { name: "ExtensionSystem" }
        Depends { name: "Utils" }
        cpp.includePaths: [
            "../..",
            "../../libs",
            product.buildDirectory + "/.obj/Core/actionmanager"
        ]
    }
}

