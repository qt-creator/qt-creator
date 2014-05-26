import qbs 1.0
import qbs.FileInfo
import QtcPlugin

QtcPlugin {
    name: "Core"

    Depends {
        name: "Qt"
        submodules: [
            "widgets", "xml", "network", "script", "sql", "help", "printsupport"
        ]
    }
    Depends { name: "Utils" }
    Depends { name: "Aggregation" }

    Depends { name: "app_version_header" }

    cpp.dynamicLibraries: {
        if (qbs.targetOS.contains("windows")) return [
            "ole32",
            "user32"
        ]
    }

    cpp.frameworks: qbs.targetOS.contains("osx") ? ["AppKit"] : undefined

    Group {
        name: "General"
        files: [
            "basefilewizardfactory.cpp", "basefilewizardfactory.h",
            "core.qrc",
            "core_global.h",
            "coreconstants.h",
            "coreplugin.cpp", "coreplugin.h",
            "designmode.cpp", "designmode.h",
            "documentmanager.cpp", "documentmanager.h",
            "editmode.cpp", "editmode.h",
            "editortoolbar.cpp", "editortoolbar.h",
            "externaltool.cpp", "externaltool.h",
            "externaltoolmanager.h",
            "fancyactionbar.cpp", "fancyactionbar.h", "fancyactionbar.qrc",
            "fancytabwidget.cpp", "fancytabwidget.h",
            "featureprovider.cpp", "featureprovider.h",
            "fileiconprovider.cpp", "fileiconprovider.h",
            "fileutils.cpp", "fileutils.h",
            "findplaceholder.cpp", "findplaceholder.h",
            "generalsettings.cpp", "generalsettings.h", "generalsettings.ui",
            "generatedfile.cpp", "generatedfile.h",
            "helpmanager.cpp", "helpmanager.h",
            "icontext.cpp", "icontext.h",
            "icore.cpp", "icore.h",
            "icorelistener.cpp", "icorelistener.h",
            "id.cpp", "id.h",
            "idocument.cpp", "idocument.h",
            "idocumentfactory.h",
            "ifilewizardextension.h",
            "imode.cpp", "imode.h",
            "inavigationwidgetfactory.cpp", "inavigationwidgetfactory.h",
            "infobar.cpp", "infobar.h",
            "ioutputpane.cpp", "ioutputpane.h",
            "iversioncontrol.cpp", "iversioncontrol.h",
            "iwizardfactory.cpp", "iwizardfactory.h",
            "mainwindow.cpp", "mainwindow.h",
            "manhattanstyle.cpp", "manhattanstyle.h",
            "messagemanager.cpp", "messagemanager.h",
            "messageoutputwindow.cpp", "messageoutputwindow.h",
            "mimedatabase.cpp", "mimedatabase.h",
            "mimetypemagicdialog.cpp", "mimetypemagicdialog.h", "mimetypemagicdialog.ui",
            "mimetypesettings.cpp", "mimetypesettings.h",
            "mimetypesettingspage.ui",
            "minisplitter.cpp", "minisplitter.h",
            "modemanager.cpp", "modemanager.h",
            "navigationsubwidget.cpp", "navigationsubwidget.h",
            "navigationwidget.cpp", "navigationwidget.h",
            "outputpane.cpp", "outputpane.h",
            "outputpanemanager.cpp", "outputpanemanager.h",
            "outputwindow.cpp", "outputwindow.h",
            "patchtool.cpp", "patchtool.h",
            "plugindialog.cpp", "plugindialog.h",
            "removefiledialog.cpp", "removefiledialog.h", "removefiledialog.ui",
            "rightpane.cpp", "rightpane.h",
            "settingsdatabase.cpp", "settingsdatabase.h",
            "sidebar.cpp", "sidebar.h",
            "sidebarwidget.cpp", "sidebarwidget.h",
            "statusbarmanager.cpp", "statusbarmanager.h",
            "statusbarwidget.cpp", "statusbarwidget.h",
            "styleanimator.cpp", "styleanimator.h",
            "tabpositionindicator.cpp", "tabpositionindicator.h",
            "textdocument.cpp", "textdocument.h",
            "toolsettings.cpp", "toolsettings.h",
            "variablechooser.cpp", "variablechooser.h", "variablechooser.ui",
            "variablemanager.cpp", "variablemanager.h",
            "vcsmanager.cpp", "vcsmanager.h",
            "versiondialog.cpp", "versiondialog.h",
        ]
    }

    Group {
        name: "Action Manager"
        prefix: "actionmanager/"
        files: [
            "actioncontainer.cpp", "actioncontainer.h", "actioncontainer_p.h",
            "actionmanager.cpp", "actionmanager.h", "actionmanager_p.h",
            "command.cpp", "command.h", "command_p.h",
            "commandbutton.cpp", "commandbutton.h",
            "commandmappings.cpp", "commandmappings.h",
            "commandsfile.cpp", "commandsfile.h",
        ]
    }

    Group {
        name: "Dialogs"
        prefix: "dialogs/"
        files: [
            "addtovcsdialog.cpp", "addtovcsdialog.h", "addtovcsdialog.ui",
            "externaltoolconfig.cpp", "externaltoolconfig.h", "externaltoolconfig.ui",
            "ioptionspage.cpp", "ioptionspage.h",
            "newdialog.cpp", "newdialog.h", "newdialog.ui",
            "openwithdialog.cpp", "openwithdialog.h", "openwithdialog.ui",
            "promptoverwritedialog.cpp", "promptoverwritedialog.h",
            "readonlyfilesdialog.cpp", "readonlyfilesdialog.h", "readonlyfilesdialog.ui",
            "saveitemsdialog.cpp", "saveitemsdialog.h", "saveitemsdialog.ui",
            "settingsdialog.cpp", "settingsdialog.h",
            "shortcutsettings.cpp", "shortcutsettings.h",
        ]
    }

    Group {
        name: "Editor Manager"
        prefix: "editormanager/"
        files: [
            "BinFiles.mimetypes.xml",
            "documentmodel.cpp", "documentmodel.h",
            "editormanager.cpp", "editormanager.h",
            "editorview.cpp", "editorview.h",
            "ieditor.cpp", "ieditor.h",
            "ieditorfactory.cpp", "ieditorfactory.h",
            "iexternaleditor.cpp", "iexternaleditor.h",
            "openeditorsview.cpp", "openeditorsview.h",
            "openeditorswindow.cpp", "openeditorswindow.h",
            "systemeditor.cpp", "systemeditor.h",
        ]
    }

    Group {
        name: "Progress Manager"
        prefix: "progressmanager/"
        files: [
            "futureprogress.cpp", "futureprogress.h",
            "progressbar.cpp", "progressbar.h",
            "progressmanager.cpp", "progressmanager.h", "progressmanager_p.h",
            "progressview.cpp", "progressview.h",
        ]
    }

    Group {
        name: "ProgressManager_win"
        condition: qbs.targetOS.contains("windows")
        files: [
            "progressmanager/progressmanager_win.cpp",
        ]
    }

    Group {
        name: "ProgressManager_mac"
        condition: qbs.targetOS.contains("osx")
        files: [
            "macfullscreen.h",
            "macfullscreen.mm",
            "progressmanager/progressmanager_mac.mm",
        ]
    }

    Group {
        name: "ProgressManager_x11"
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("osx")
        files: [
            "progressmanager/progressmanager_x11.cpp",
        ]
    }

    Group {
        name: "Tests"
        condition: project.testsEnabled
        files: [
            "testdatadir.cpp",
            "testdatadir.h",
            "locator/locatorfiltertest.cpp",
            "locator/locatorfiltertest.h",
            "locator/locator_test.cpp"
        ]

        cpp.defines: outer.concat(['SRCDIR="' + FileInfo.path(filePath) + '"'])
    }

    Group {
        name: "Find"
        prefix: "find/"
        files: [
            "basetextfind.cpp",
            "basetextfind.h",
            "currentdocumentfind.cpp",
            "currentdocumentfind.h",
            "find.qrc",
            "finddialog.ui",
            "findplugin.cpp",
            "findplugin.h",
            "findtoolbar.cpp",
            "findtoolbar.h",
            "findtoolwindow.cpp",
            "findtoolwindow.h",
            "findwidget.ui",
            "ifindfilter.cpp",
            "ifindfilter.h",
            "ifindsupport.cpp",
            "ifindsupport.h",
            "searchresultcolor.h",
            "searchresulttreeitemdelegate.cpp",
            "searchresulttreeitemdelegate.h",
            "searchresulttreeitemroles.h",
            "searchresulttreeitems.cpp",
            "searchresulttreeitems.h",
            "searchresulttreemodel.cpp",
            "searchresulttreemodel.h",
            "searchresulttreeview.cpp",
            "searchresulttreeview.h",
            "searchresultwidget.cpp",
            "searchresultwidget.h",
            "searchresultwindow.cpp",
            "searchresultwindow.h",
            "textfindconstants.h",
            "treeviewfind.cpp",
            "treeviewfind.h",
        ]
    }

    Group {
        name: "Locator"
        prefix: "locator/"
        files: [
            "basefilefilter.cpp",
            "basefilefilter.h",
            "commandlocator.cpp",
            "commandlocator.h",
            "directoryfilter.cpp",
            "directoryfilter.h",
            "directoryfilter.ui",
            "executefilter.cpp",
            "executefilter.h",
            "filesystemfilter.cpp",
            "filesystemfilter.h",
            "filesystemfilter.ui",
            "ilocatorfilter.cpp",
            "ilocatorfilter.h",
            "locator.qrc",
            "locatorconstants.h",
            "locatorfiltersfilter.cpp",
            "locatorfiltersfilter.h",
            "locatormanager.cpp",
            "locatormanager.h",
            "locator.cpp",
            "locator.h",
            "locatorsearchutils.cpp",
            "locatorsearchutils.h",
            "locatorwidget.cpp",
            "locatorwidget.h",
            "opendocumentsfilter.cpp",
            "opendocumentsfilter.h",
            "settingspage.cpp",
            "settingspage.h",
            "settingspage.ui",
            "images/locator.png",
            "images/reload.png",
        ]
    }

    Export {
        Depends { name: "Aggregation" }
        Depends { name: "Utils" }
    }
}
