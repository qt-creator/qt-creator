import qbs
import QtcAutotest

QtcAutotest {
    name: "ExternalTool autotest"
    property path corePluginDir: project.ide_source_tree + "/src/plugins/coreplugin"
    // TODO: This should be all that is needed here: Depends { name: "Core" }
    Depends { name: "app_version_header" }
    Depends { name: "Aggregation" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "Utils" }
    Depends { name: "Qt"; submodules: ["help", "printsupport", "sql"]; }
    Depends { name: "Qt.widgets" } // TODO: qbs bug, remove when fixed
    Group {
        name: "Sources from Core plugin"
        prefix: product.corePluginDir + '/'
        files: [
            "actionmanager/*",
            "dialogs/*",
            "editormanager/*",
            "documentmanager.h", "documentmanager.cpp",
            "editortoolbar.h", "editortoolbar.cpp",
            "externaltool.h", "externaltool.cpp",
            "externaltoolmanager.h",
            "fancyactionbar.h", "fancyactionbar.cpp",
            "fancytabwidget.h", "fancytabwidget.cpp",
            "featureprovider.h", "featureprovider.cpp",
            "fileiconprovider.h", "fileiconprovider.cpp",
            "fileutils.h", "fileutils.cpp",
            "findplaceholder.h", "findplaceholder.cpp",
            "generalsettings.*",
            "helpmanager.h", "helpmanager.cpp",
            "icontext.h", "icontext.cpp",
            "icore.h", "icore.cpp",
            "icorelistener.h",
            "idocument.h", "idocument.cpp",
            "idocumentfactory.h",
            "id.h", "id.cpp",
            "imode.h", "imode.cpp",
            "inavigationwidgetfactory.h", "inavigationwidgetfactory.cpp",
            "infobar.h", "infobar.cpp",
            "ioutputpane.h",
            "iversioncontrol.h", "iversioncontrol.cpp",
            "mainwindow.h", "mainwindow.cpp",
            "manhattanstyle.h", "manhattanstyle.cpp",
            "messagemanager.h", "messagemanager.cpp",
            "messageoutputwindow.h", "messageoutputwindow.cpp",
            "mimedatabase.h", "mimedatabase.cpp",
            "mimetypemagicdialog.*",
            "mimetypesettings.h", "mimetypesettings.cpp",
            "minisplitter.h", "minisplitter.cpp",
            "modemanager.h", "modemanager.cpp",
            "navigationsubwidget.h", "navigationsubwidget.cpp",
            "navigationwidget.h", "navigationwidget.cpp",
            "outputpane.h", "outputpane.cpp",
            "outputpanemanager.h", "outputpanemanager.cpp",
            "outputwindow.h", "outputwindow.cpp",
            "plugindialog.h", "plugindialog.cpp",
            "rightpane.h", "rightpane.cpp",
            "settingsdatabase.h", "settingsdatabase.cpp",
            "statusbarmanager.h", "statusbarmanager.cpp",
            "statusbarwidget.h", "statusbarwidget.cpp",
            "styleanimator.h", "styleanimator.cpp",
            "toolsettings.h", "toolsettings.cpp",
            "variablechooser.h", "variablechooser.cpp",
            "variablemanager.h", "variablemanager.cpp",
            "vcsmanager.h", "vcsmanager.cpp",
            "versiondialog.h", "versiondialog.cpp",
        ]
    }

    Group {
        name: "Find"
        prefix: product.corePluginDir + "/find/"
        files: [ "*.cpp", "*.h" ]
    }

    Group {
        name: "Progress Manager"
        prefix: product.corePluginDir + "/progressmanager/"
        files: [
            "futureprogress.cpp", "futureprogress.h",
            "progressbar.cpp", "progressbar.h",
            "progressmanager.cpp", "progressmanager.h", "progressmanager_p.h",
            "progressview.cpp", "progressview.h",
        ]
    }

    Group {
        name: "ProgressManager_win"
        prefix: product.corePluginDir + '/'
        condition: qbs.targetOS.contains("windows")
        files: [
            "progressmanager/progressmanager_win.cpp",
        ]
    }

    Group {
        name: "ProgressManager_mac"
        prefix: product.corePluginDir + '/'
        condition: qbs.targetOS.contains("osx")
        files: [
            "macfullscreen.h",
            "macfullscreen.mm",
            "progressmanager/progressmanager_mac.mm",
        ]
    }

    Group {
        name: "ProgressManager_x11"
        prefix: product.corePluginDir + '/'
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("osx")
        files: [
            "progressmanager/progressmanager_x11.cpp",
        ]
    }

    Group {
        name: "Test sources"
        files: "tst_externaltooltest.cpp"
    }
    cpp.defines: base.concat([
        "QT_DISABLE_DEPRECATED_BEFORE=0x040900",
        "CORE_LIBRARY" // Needed to compile on Windows...
    ])
    cpp.includePaths: base.concat([
        product.buildDirectory + "/GeneratedFiles/Core",
        corePluginDir + "/.."
    ])
    cpp.dynamicLibraries: {
        if (qbs.targetOS.contains("windows")) return [
            "ole32",
            "user32"
        ]
    }
    cpp.frameworks: qbs.targetOS.contains("osx") ? ["AppKit"] : undefined
}
