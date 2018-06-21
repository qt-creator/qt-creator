import qbs 1.0

QtcPlugin {
    name: "ClassView"

    Depends { name: "Qt.widgets" }
    Depends { name: "CPlusPlus" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }

    files: [
        "classviewconstants.h",
        "classviewmanager.cpp",
        "classviewmanager.h",
        "classviewnavigationwidget.cpp",
        "classviewnavigationwidget.h",
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

