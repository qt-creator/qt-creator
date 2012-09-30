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

    Depends { name: "cpp" }
    Properties {
        condition: qtcore.versionMajor >= 5
        cpp.defines: base.concat(["QT_NO_WEBKIT"])
    }
    cpp.defines: base.concat(["QT_CLUCENE_SUPPORT"])
    cpp.includePaths: [
        "../../shared/help",
        ".",
        "..",
        "../..",
        "../../libs"
    ]

    files: [
        "centralwidget.h",
        "docsettingspage.h",
        "filtersettingspage.h",
        "generalsettingspage.h",
        "help_global.h",
        "helpconstants.h",
        "helpfindsupport.h",
        "helpindexfilter.h",
        "localhelpmanager.h",
        "helpmode.h",
        "helpplugin.h",
        "helpviewer.h",
        "helpviewer_p.h",
        "openpagesmanager.h",
        "openpagesmodel.h",
        "openpagesswitcher.h",
        "openpageswidget.h",
        "remotehelpfilter.h",
        "searchwidget.h",
        "xbelsupport.h",
        "externalhelpwindow.h",
        "centralwidget.cpp",
        "docsettingspage.cpp",
        "filtersettingspage.cpp",
        "generalsettingspage.cpp",
        "helpfindsupport.cpp",
        "helpindexfilter.cpp",
        "localhelpmanager.cpp",
        "helpmode.cpp",
        "helpplugin.cpp",
        "helpviewer.cpp",
        "helpviewer_qtb.cpp",
        "helpviewer_qwv.cpp",
        "openpagesmanager.cpp",
        "openpagesmodel.cpp",
        "openpagesswitcher.cpp",
        "openpageswidget.cpp",
        "remotehelpfilter.cpp",
        "searchwidget.cpp",
        "xbelsupport.cpp",
        "externalhelpwindow.cpp",
        "docsettingspage.ui",
        "filtersettingspage.ui",
        "generalsettingspage.ui",
        "remotehelpfilter.ui",
        "help.qrc",
        "../../shared/help/bookmarkmanager.h",
        "../../shared/help/contentwindow.h",
        "../../shared/help/filternamedialog.h",
        "../../shared/help/indexwindow.h",
        "../../shared/help/topicchooser.h",
        "../../shared/help/bookmarkmanager.cpp",
        "../../shared/help/contentwindow.cpp",
        "../../shared/help/filternamedialog.cpp",
        "../../shared/help/indexwindow.cpp",
        "../../shared/help/topicchooser.cpp",
        "../../shared/help/bookmarkdialog.ui",
        "../../shared/help/filternamedialog.ui",
        "../../shared/help/topicchooser.ui"
    ]
}

