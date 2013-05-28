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


    files: [
        "classview.qrc",
        "classviewconstants.h",
        "classviewmanager.cpp",
        "classviewmanager.h",
        "classviewnavigationwidget.cpp",
        "classviewnavigationwidget.h",
        "classviewnavigationwidget.ui",
        "classviewnavigationwidgetfactory.cpp",
        "classviewnavigationwidgetfactory.h",
        "classviewparser.cpp",
        "classviewparser.h",
        "classviewparsertreeitem.cpp",
        "classviewparsertreeitem.h",
        "classviewplugin.cpp",
        "classviewplugin.h",
        "classviewsymbolinformation.cpp",
        "classviewsymbolinformation.h",
        "classviewsymbollocation.cpp",
        "classviewsymbollocation.h",
        "classviewtreeitemmodel.cpp",
        "classviewtreeitemmodel.h",
        "classviewutils.cpp",
        "classviewutils.h",
    ]
}

