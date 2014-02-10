import qbs.base 1.0

import QtcPlugin

QtcPlugin {
    name: "Help"

    Depends { name: "Qt"; submodules: ["help", "network", "webkit"]; }
    Depends {
        condition: Qt.core.versionMajor >= 5;
        name: "Qt"; submodules: ["printsupport", "webkitwidgets"];
    }
    Depends { name: "Aggregation" }
    Depends { name: "Utils" }

    Depends { name: "Core" }

    Depends { name: "app_version_header" }

    cpp.defines: base.concat(["QT_CLUCENE_SUPPORT"])

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
            "helpviewer.cpp", "helpviewer.h", "helpviewer_p.h",
            "helpviewer_qtb.cpp",
            "helpviewer_qwv.cpp",
            "localhelpmanager.cpp", "localhelpmanager.h",
            "openpagesmanager.cpp", "openpagesmanager.h",
            "openpagesmodel.cpp", "openpagesmodel.h",
            "openpagesswitcher.cpp", "openpagesswitcher.h",
            "openpageswidget.cpp", "openpageswidget.h",
            "remotehelpfilter.cpp", "remotehelpfilter.h", "remotehelpfilter.ui",
            "searchwidget.cpp", "searchwidget.h",
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
