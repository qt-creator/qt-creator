import qbs 1.0

import QtcPlugin

QtcPlugin {
    name: "Help"

    Depends { name: "Qt"; submodules: ["help", "network"]; }
    Depends {
        condition: Qt.core.versionMajor >= 5;
        name: "Qt.printsupport"
    }
    Depends {
        name: "Qt.webkit"
        required: false
    }
    Depends {
        name: "Qt.webkitwidgets"
        condition: Qt.core.versionMajor >= 5 && Qt.webkit.present
    }

    Depends { name: "Aggregation" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }

    Depends { name: "app_version_header" }

    cpp.defines: {
        var defines = base.concat(["QT_CLUCENE_SUPPORT"]);
        if (Qt.core.versionMajor >= 5 && !Qt.webkit.present)
            defines.push("QT_NO_WEBKIT");
        return defines;
    }

    // We include headers from src/shared/help, and their sources include headers from here...
    cpp.includePaths: base.concat([sharedSources.prefix, path])

    Group {
        name: "Sources"
        files: [
            "centralwidget.cpp", "centralwidget.h",
            "docsettingspage.cpp", "docsettingspage.h", "docsettingspage.ui",
            "externalhelpwindow.cpp", "externalhelpwindow.h",
            "filtersettingspage.cpp", "filtersettingspage.h", "filtersettingspage.ui",
            "generalsettingspage.cpp", "generalsettingspage.h", "generalsettingspage.ui",
            "help.qrc",
            "helpconstants.h",
            "helpfindsupport.cpp", "helpfindsupport.h",
            "helpindexfilter.cpp", "helpindexfilter.h",
            "helpmode.cpp", "helpmode.h",
            "helpplugin.cpp", "helpplugin.h",
            "helpviewer.cpp", "helpviewer.h",
            "localhelpmanager.cpp", "localhelpmanager.h",
            "openpagesmanager.cpp", "openpagesmanager.h",
            "openpagesmodel.cpp", "openpagesmodel.h",
            "openpagesswitcher.cpp", "openpagesswitcher.h",
            "openpageswidget.cpp", "openpageswidget.h",
            "qtwebkithelpviewer.cpp", "qtwebkithelpviewer.h",
            "remotehelpfilter.cpp", "remotehelpfilter.h", "remotehelpfilter.ui",
            "searchtaskhandler.cpp", "searchtaskhandler.h",
            "searchwidget.cpp", "searchwidget.h",
            "textbrowserhelpviewer.cpp", "textbrowserhelpviewer.h",
            "xbelsupport.cpp", "xbelsupport.h",
        ]
    }

    Group {
        id: sharedSources
        name: "Shared Sources"
        prefix: "../../shared/help/"
        files: [
            "bookmarkdialog.ui",
            "bookmarkmanager.cpp", "bookmarkmanager.h",
            "contentwindow.cpp", "contentwindow.h",
            "filternamedialog.cpp", "filternamedialog.h", "filternamedialog.ui",
            "indexwindow.cpp", "indexwindow.h",
            "topicchooser.cpp", "topicchooser.h", "topicchooser.ui",
        ]
    }
}
