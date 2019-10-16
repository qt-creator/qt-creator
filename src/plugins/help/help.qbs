import qbs.Utilities

Project {
    name: "Help"

    references: "qlitehtml"

    QtcPlugin {
        name: "Help"

        Depends { name: "Qt"; submodules: ["help", "network", "sql"]; }
        Depends { name: "Qt.printsupport" }
        Depends { name: "Qt.webenginewidgets"; required: false }

        Depends { name: "Aggregation" }
        Depends { name: "Utils" }

        Depends { name: "Core" }
        Depends { name: "ProjectExplorer" }

        Depends { name: "app_version_header" }

        Depends { name: "qlitehtml"; required: false }

        cpp.defines: {
            var defines = base.concat(["QT_CLUCENE_SUPPORT"]);
            if (Qt.webenginewidgets.present)
                defines.push("QTC_WEBENGINE_HELPVIEWER");
            if (qlitehtml.present)
                defines.push("QTC_LITEHTML_HELPVIEWER")
            if (Utilities.versionCompare(Qt.core.version, "5.15") >= 0)
                defines.push("HELP_NEW_FILTER_ENGINE");
            return defines;
        }

        // We include headers from src/shared/help, and their sources include headers from here...
        cpp.includePaths: base.concat([sharedSources.prefix, path])

        Group {
            name: "Sources"
            files: [
                "docsettingspage.cpp", "docsettingspage.h", "docsettingspage.ui",
                "filtersettingspage.cpp", "filtersettingspage.h", "filtersettingspage.ui",
                "generalsettingspage.cpp", "generalsettingspage.h", "generalsettingspage.ui",
                "help.qrc",
                "helpconstants.h",
                "helpfindsupport.cpp", "helpfindsupport.h",
                "helpindexfilter.cpp", "helpindexfilter.h",
                "helpmanager.cpp", "helpmanager.h",
                "helpmode.cpp", "helpmode.h",
                "helpplugin.cpp", "helpplugin.h",
                "helpviewer.cpp", "helpviewer.h",
                "helpwidget.cpp", "helpwidget.h",
                "localhelpmanager.cpp", "localhelpmanager.h",
                "openpagesmanager.cpp", "openpagesmanager.h",
                "openpagesswitcher.cpp", "openpagesswitcher.h",
                "openpageswidget.cpp", "openpageswidget.h",
                "searchtaskhandler.cpp", "searchtaskhandler.h",
                "searchwidget.cpp", "searchwidget.h",
                "textbrowserhelpviewer.cpp", "textbrowserhelpviewer.h",
                "xbelsupport.cpp", "xbelsupport.h",
            ]
        }

        Group {
            name: "WebEngine Sources"
            condition: Qt.webenginewidgets.present
            files: [
                "webenginehelpviewer.cpp", "webenginehelpviewer.h"
            ]
        }

        Group {
            name: "litehtml-specific sources"
            condition: qlitehtml.present
            cpp.warningLevel: "none"
            files: [
                "litehtmlhelpviewer.cpp",
                "litehtmlhelpviewer.h",
            ]
        }

        Group {
            id: sharedSources
            name: "Shared Sources"
            prefix: project.sharedSourcesDir + "/help/"
            files: [
                "bookmarkdialog.ui",
                "bookmarkmanager.cpp", "bookmarkmanager.h",
                "contentwindow.cpp", "contentwindow.h",
                "filternamedialog.cpp", "filternamedialog.h", "filternamedialog.ui",
                "helpicons.h",
                "indexwindow.cpp", "indexwindow.h",
                "topicchooser.cpp", "topicchooser.h", "topicchooser.ui",
            ]
        }
    }
}
