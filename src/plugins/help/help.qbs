import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Help"

    condition: qtcore.versionMajor === 4
    Depends { id: qtcore; name: "qt.core" }
    Depends { name: "qt"; submodules: ['widgets', 'help', 'webkit', 'network'] }
    Depends { name: "Core" }
    Depends { name: "Find" }
    Depends { name: "Locator" }

    Depends { name: "cpp" }
    cpp.defines: base.concat(["QT_CLUCENE_SUPPORT"])
    cpp.includePaths: [
        "../../shared/help",
        ".",
        "..",
        "../..",
        "../../libs",
        buildDirectory
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

