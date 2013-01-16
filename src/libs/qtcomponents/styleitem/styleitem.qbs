import qbs.base 1.0
import "../../../../qbs/defaults.js" as Defaults

DynamicLibrary {
    name: "styleplugin"

    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ["core", "widgets", "declarative", "script"] }

    cpp.defines: Defaults.defines(qbs)

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
        "qwheelarea.h",
    ]

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: "lib/qtcreator/qtcomponents/plugin"
    }
}
