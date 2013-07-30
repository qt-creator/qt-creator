import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Help"

    Depends { id: qtcore; name: "Qt.core" }
    Depends {
        condition: qtcore.versionMajor == 4
        name: "Qt"; submodules: ["widgets", "help", "webkit", "network"]
    }
    Depends {
        condition: qtcore.versionMajor >= 5
        name: "Qt"; submodules: ["widgets", "help", "network", "printsupport"]
    }

    Depends { name: "Core" }
    Depends { name: "Find" }
    Depends { name: "Locator" }
    Depends { name: "app_version_header" }


    cpp.defines: {
        var list = base;
        if (qtcore.versionMajor >= 5)
            list.push("QT_NO_WEBKIT");
        list.push("QT_CLUCENE_SUPPORT");
        return list;
    }

    cpp.includePaths: base.concat(sharedSources.prefix)

    Group {
        name: "Sources"
        files: [
            "centralwidget.cpp", "centralwidget.h",
            "docsettingspage.cpp", "docsettingspage.h", "docsettingspage.ui",
            "externalhelpwindow.cpp", "externalhelpwindow.h",
            "filtersettingspage.cpp", "filtersettingspage.h", "filtersettingspage.ui",
            "generalsettingspage.cpp", "generalsettingspage.h", "generalsettingspage.ui",
            "help.qrc",
            "help_global.h",
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
