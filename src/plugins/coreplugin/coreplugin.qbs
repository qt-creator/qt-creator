import qbs.FileInfo
import qbs.Utilities

QtcPlugin {
    name: "Core"
    Depends {
        name: "Qt"
        submodules: ["widgets", "xml", "network", "qml", "sql", "printsupport"]
    }

    Depends {
        name: "Qt.gui-private"
        condition: qbs.targetOS.contains("windows")
    }

    Depends { name: "Utils" }
    Depends { name: "Aggregation" }
    Depends { name: "TerminalLib" }

    cpp.dynamicLibraries: {
        if (qbs.targetOS.contains("windows"))
            return ["ole32", "user32"]
    }

    cpp.frameworks: qbs.targetOS.contains("macos") ? ["AppKit"] : undefined

    Group {
        name: "General"
        files: [
            "actionsfilter.cpp",
            "actionsfilter.h",
            "basefilewizard.cpp",
            "basefilewizard.h",
            "basefilewizardfactory.cpp",
            "basefilewizardfactory.h",
            "core.qrc",
            "core_global.h",
            "coreconstants.h",
            "coreicons.cpp",
            "coreicons.h",
            "corejsextensions.cpp",
            "corejsextensions.h",
            "coreplugin.cpp",
            "coreplugin.h",
            "coreplugintr.h",
            "designmode.cpp",
            "designmode.h",
            "diffservice.cpp",
            "diffservice.h",
            "documentmanager.cpp",
            "documentmanager.h",
            "editmode.cpp",
            "editmode.h",
            "editortoolbar.cpp",
            "editortoolbar.h",
            "externaltool.cpp",
            "externaltool.h",
            "externaltoolmanager.cpp",
            "externaltoolmanager.h",
            "fancyactionbar.cpp",
            "fancyactionbar.h",
            "fancyactionbar.qrc",
            "fancytabwidget.cpp",
            "fancytabwidget.h",
            "featureprovider.cpp",
            "featureprovider.h",
            "fileutils.cpp",
            "fileutils.h",
            "findplaceholder.cpp",
            "findplaceholder.h",
            "foldernavigationwidget.cpp",
            "foldernavigationwidget.h",
            "generalsettings.cpp",
            "generalsettings.h",
            "generatedfile.cpp",
            "generatedfile.h",
            "helpitem.cpp",
            "helpitem.h",
            "helpmanager.cpp",
            "helpmanager.h",
            "helpmanager_implementation.h",
            "icontext.cpp",
            "icontext.h",
            "icore.cpp",
            "icore.h",
            "idocument.cpp",
            "idocument.h",
            "idocumentfactory.cpp",
            "idocumentfactory.h",
            "ifilewizardextension.h",
            "imode.cpp",
            "imode.h",
            "inavigationwidgetfactory.cpp",
            "inavigationwidgetfactory.h",
            "ioutputpane.cpp",
            "ioutputpane.h",
            "iversioncontrol.cpp",
            "iversioncontrol.h",
            "iwelcomepage.cpp",
            "iwelcomepage.h",
            "iwizardfactory.cpp",
            "iwizardfactory.h",
            "jsexpander.cpp",
            "jsexpander.h",
            "loggingviewer.cpp",
            "loggingviewer.h",
            "manhattanstyle.cpp",
            "manhattanstyle.h",
            "messagebox.cpp",
            "messagebox.h",
            "messagemanager.cpp",
            "messagemanager.h",
            "messageoutputwindow.cpp",
            "messageoutputwindow.h",
            "mimetypemagicdialog.cpp",
            "mimetypemagicdialog.h",
            "mimetypesettings.cpp",
            "mimetypesettings.h",
            "minisplitter.cpp",
            "minisplitter.h",
            "modemanager.cpp",
            "modemanager.h",
            "navigationsubwidget.cpp",
            "navigationsubwidget.h",
            "navigationwidget.cpp",
            "navigationwidget.h",
            "opendocumentstreeview.cpp",
            "opendocumentstreeview.h",
            "outputpane.cpp",
            "outputpane.h",
            "outputpanemanager.cpp",
            "outputpanemanager.h",
            "outputwindow.cpp",
            "outputwindow.h",
            "patchtool.cpp",
            "patchtool.h",
            "plugindialog.cpp",
            "plugindialog.h",
            "plugininstallwizard.cpp",
            "plugininstallwizard.h",
            "rightpane.cpp",
            "rightpane.h",
            "session.cpp",
            "session.h",
            "sessiondialog.cpp",
            "sessiondialog.h",
            "sessionmodel.cpp",
            "sessionmodel.h",
            "sessionview.cpp",
            "sessionview.h",
            "settingsdatabase.cpp",
            "settingsdatabase.h",
            "sidebar.cpp",
            "sidebar.h",
            "sidebarwidget.cpp",
            "sidebarwidget.h",
            "statusbarmanager.cpp",
            "statusbarmanager.h",
            "systemsettings.cpp",
            "systemsettings.h",
            "textdocument.cpp",
            "textdocument.h",
            "themechooser.cpp",
            "themechooser.h",
            "vcsmanager.cpp",
            "vcsmanager.h",
            "versiondialog.cpp",
            "versiondialog.h",
            "welcomepagehelper.cpp",
            "welcomepagehelper.h",
            "windowsupport.cpp",
            "windowsupport.h",
        ]
    }

    Group {
        name: "studiofonts"
        prefix: "../../share/3rdparty/studiofonts/"
        files: "studiofonts.qrc"
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
            "addtovcsdialog.cpp", "addtovcsdialog.h",
            "codecselector.cpp", "codecselector.h",
            "externaltoolconfig.cpp", "externaltoolconfig.h",
            "filepropertiesdialog.cpp", "filepropertiesdialog.h",
            "ioptionspage.cpp", "ioptionspage.h",
            "newdialog.cpp", "newdialog.h",
            "newdialogwidget.cpp", "newdialogwidget.h",
            "openwithdialog.cpp", "openwithdialog.h",
            "promptoverwritedialog.cpp", "promptoverwritedialog.h",
            "readonlyfilesdialog.cpp", "readonlyfilesdialog.h",
            "restartdialog.cpp", "restartdialog.h",
            "saveitemsdialog.cpp", "saveitemsdialog.h",
            "settingsdialog.cpp", "settingsdialog.h",
            "shortcutsettings.cpp", "shortcutsettings.h",
        ]
    }

    Group {
        name: "Editor Manager"
        prefix: "editormanager/"
        files: [
            "documentmodel.cpp", "documentmodel.h", "documentmodel_p.h",
            "editorarea.cpp", "editorarea.h",
            "editormanager.cpp", "editormanager.h", "editormanager_p.h",
            "editorview.cpp", "editorview.h",
            "editorwindow.cpp", "editorwindow.h",
            "ieditor.cpp", "ieditor.h",
            "ieditorfactory.cpp", "ieditorfactory.h", "ieditorfactory_p.h",
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
            "processprogress.cpp", "processprogress.h",
            "progressbar.cpp", "progressbar.h",
            "progressmanager.cpp", "progressmanager.h", "progressmanager_p.h",
            "progressview.cpp", "progressview.h",
            "taskprogress.cpp", "taskprogress.h",
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
        condition: qbs.targetOS.contains("macos")
        files: [
            "progressmanager/progressmanager_mac.mm",
        ]
    }

    Group {
        name: "ProgressManager_x11"
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("macos")
        files: [
            "progressmanager/progressmanager_x11.cpp",
        ]
    }

    QtcTestFiles {
        files: [
            "testdatadir.cpp",
            "testdatadir.h",
            "locator/locatorfiltertest.cpp",
            "locator/locatorfiltertest.h",
            "locator/locator_test.cpp"
        ]

        cpp.defines: outer.concat(['SRCDIR="' + path + '"'])
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
            "findplugin.cpp",
            "findplugin.h",
            "findtoolbar.cpp",
            "findtoolbar.h",
            "findtoolwindow.cpp",
            "findtoolwindow.h",
            "highlightscrollbarcontroller.cpp",
            "highlightscrollbarcontroller.h",
            "ifindfilter.cpp",
            "ifindfilter.h",
            "ifindsupport.cpp",
            "ifindsupport.h",
            "itemviewfind.cpp",
            "itemviewfind.h",
            "optionspopup.cpp",
            "optionspopup.h",
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
        ]
    }

    Group {
        name: "Locator"
        prefix: "locator/"
        files: [
            "commandlocator.cpp",
            "commandlocator.h",
            "directoryfilter.cpp",
            "directoryfilter.h",
            "executefilter.cpp",
            "executefilter.h",
            "externaltoolsfilter.cpp",
            "externaltoolsfilter.h",
            "filesystemfilter.cpp",
            "filesystemfilter.h",
            "ilocatorfilter.cpp",
            "ilocatorfilter.h",
            "javascriptfilter.cpp",
            "javascriptfilter.h",
            "locatorconstants.h",
            "locatorfiltersfilter.cpp",
            "locatorfiltersfilter.h",
            "locatormanager.cpp",
            "locatormanager.h",
            "locator.cpp",
            "locator.h",
            "locatorsettingspage.cpp",
            "locatorsettingspage.h",
            "locatorwidget.cpp",
            "locatorwidget.h",
            "opendocumentsfilter.cpp",
            "opendocumentsfilter.h",
            "spotlightlocatorfilter.h",
            "spotlightlocatorfilter.cpp",
            "urllocatorfilter.cpp",
            "urllocatorfilter.h"
        ]
    }

    Group {
        name: "Terminal"
        prefix: "terminal/"
        files: [
            "searchableterminal.cpp",
            "searchableterminal.h",
        ]
    }

    Export {
        Depends { name: "Aggregation" }
        Depends { name: "Utils" }
    }
}
