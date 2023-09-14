import qbs.Utilities

QtcPlugin {
    name: "Help"

    Depends { name: "Qt"; submodules: ["help", "network", "sql"] }
    Depends { name: "Qt.printsupport" }
    Depends { name: "Qt.webenginewidgets"; required: false }

    Depends { name: "Aggregation" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

    Depends { name: "qlitehtml"; required: false }

    cpp.defines: {
        var defines = base.concat(["QT_CLUCENE_SUPPORT"]);
        if (Qt.webenginewidgets.present)
            defines.push("QTC_WEBENGINE_HELPVIEWER");
        if (qlitehtml.present)
            defines.push("QTC_LITEHTML_HELPVIEWER")
        return defines;
    }

    // We include headers from src/shared/help, and their sources include headers from here...
    cpp.includePaths: base.concat([sharedSources.prefix, path])

    Group {
        name: "Sources"
        files: [
            "docsettingspage.cpp", "docsettingspage.h",
            "filtersettingspage.cpp", "filtersettingspage.h",
            "generalsettingspage.cpp", "generalsettingspage.h",
            "help.qrc",
            "helpconstants.h",
            "helpfindsupport.cpp", "helpfindsupport.h",
            "helpindexfilter.cpp", "helpindexfilter.h",
            "helpmanager.cpp", "helpmanager.h",
            "helpplugin.cpp", "helpplugin.h",
            "helpviewer.cpp", "helpviewer.h",
            "helpwidget.cpp", "helpwidget.h",
            "helptr.h",
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
            "bookmarkmanager.cpp", "bookmarkmanager.h",
            "contentwindow.cpp", "contentwindow.h",
            "helpicons.h",
            "indexwindow.cpp", "indexwindow.h",
            "topicchooser.cpp", "topicchooser.h",
        ]
    }
}
