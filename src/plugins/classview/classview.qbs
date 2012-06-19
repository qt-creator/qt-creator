import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "ClassView"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "CPlusPlus" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }

    Depends { name: "cpp" }
    cpp.includePaths: [
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "classviewplugin.h",
        "classviewnavigationwidgetfactory.h",
        "classviewconstants.h",
        "classviewnavigationwidget.h",
        "classviewparser.h",
        "classviewmanager.h",
        "classviewsymbollocation.h",
        "classviewsymbolinformation.h",
        "classviewparsertreeitem.h",
        "classviewutils.h",
        "classviewtreeitemmodel.h",
        "classviewplugin.cpp",
        "classviewnavigationwidgetfactory.cpp",
        "classviewnavigationwidget.cpp",
        "classviewparser.cpp",
        "classviewmanager.cpp",
        "classviewsymbollocation.cpp",
        "classviewsymbolinformation.cpp",
        "classviewparsertreeitem.cpp",
        "classviewutils.cpp",
        "classviewtreeitemmodel.cpp",
        "classviewnavigationwidget.ui",
        "classview.qrc",
    ]
}

