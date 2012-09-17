import qbs.base 1.0

DynamicLibrary {
    name: "styleplugin"
    destination: "lib/qtcreator/qtcomponents/plugin"

    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ["core", "widgets", "quick1", "script"] }

    cpp.defines: project.additionalCppDefines

    files: [
        "qdeclarativefolderlistmodel.cpp",
        "qdeclarativefolderlistmodel.h",
        "qrangemodel.cpp",
        "qrangemodel.h",
        "qrangemodel_p.h",
        "qstyleitem.cpp",
        "qstyleitem.h",
        "qstyleplugin.cpp",
        "qstyleplugin.h",
        "qtmenu.cpp",
        "qtmenu.h",
        "qtmenubar.cpp",
        "qtmenubar.h",
        "qtmenuitem.cpp",
        "qtmenuitem.h",
        "qwheelarea.cpp",
        "qwheelarea.h"
    ]
}

