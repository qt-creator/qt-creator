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
    cpp.defines: base.concat("QT_CLUCENE_SUPPORT")
    cpp.includePaths: base.concat("../../shared/help")

    files: [
        "centralwidget.cpp",
        "centralwidget.h",
        "docsettingspage.cpp",
        "docsettingspage.h",
        "docsettingspage.ui",
        "externalhelpwindow.cpp",
        "externalhelpwindow.h",
        "filtersettingspage.cpp",
        "filtersettingspage.h",
        "filtersettingspage.ui",
        "generalsettingspage.cpp",
        "generalsettingspage.h",
        "generalsettingspage.ui",
        "help.qrc",
        "help_global.h",
        "helpconstants.h",
        "helpfindsupport.cpp",
        "helpfindsupport.h",
        "helpindexfilter.cpp",
        "helpindexfilter.h",
        "helpmode.cpp",
        "helpmode.h",
        "helpplugin.cpp",
        "helpplugin.h",
        "helpviewer.cpp",
        "helpviewer.h",
        "helpviewer_p.h",
        "helpviewer_qtb.cpp",
        "helpviewer_qwv.cpp",
        "localhelpmanager.cpp",
        "localhelpmanager.h",
        "openpagesmanager.cpp",
        "openpagesmanager.h",
        "openpagesmodel.cpp",
        "openpagesmodel.h",
        "openpagesswitcher.cpp",
        "openpagesswitcher.h",
        "openpageswidget.cpp",
        "openpageswidget.h",
        "remotehelpfilter.cpp",
        "remotehelpfilter.h",
        "remotehelpfilter.ui",
        "searchwidget.cpp",
        "searchwidget.h",
        "xbelsupport.cpp",
        "xbelsupport.h",
        "../../shared/help/bookmarkdialog.ui",
        "../../shared/help/bookmarkmanager.cpp",
        "../../shared/help/bookmarkmanager.h",
        "../../shared/help/contentwindow.cpp",
        "../../shared/help/contentwindow.h",
        "../../shared/help/filternamedialog.cpp",
        "../../shared/help/filternamedialog.h",
        "../../shared/help/filternamedialog.ui",
        "../../shared/help/indexwindow.cpp",
        "../../shared/help/indexwindow.h",
        "../../shared/help/topicchooser.cpp",
        "../../shared/help/topicchooser.h",
        "../../shared/help/topicchooser.ui",
    ]
}
