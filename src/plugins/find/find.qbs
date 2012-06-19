import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Find"

    Depends { name: "Qt"; submodules: ["widgets", "xml", "network", "script"] }
    Depends { name: "Core" }
    Depends { name: "cpp" }

    cpp.includePaths: [
        ".",
        "..",
        "generichighlighter",
        "tooltip",
        "snippets",
        "codeassist",
        "../../libs",
        buildDirectory
    ]

    files: [
        "find.qrc",
        "findwidget.ui",
        "basetextfind.cpp",
        "basetextfind.h",
        "currentdocumentfind.cpp",
        "currentdocumentfind.h",
        "find_global.h",
        "finddialog.ui",
        "findplugin.cpp",
        "findplugin.h",
        "findtoolbar.cpp",
        "findtoolbar.h",
        "findtoolwindow.cpp",
        "findtoolwindow.h",
        "ifindfilter.cpp",
        "ifindfilter.h",
        "ifindsupport.cpp",
        "ifindsupport.h",
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
        "treeviewfind.cpp",
        "treeviewfind.h"
    ]
}

