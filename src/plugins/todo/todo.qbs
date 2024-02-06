import qbs 1.0

QtcPlugin {
    name: "Todo"

    Depends { name: "Qt.widgets" }
    Depends { name: "CPlusPlus" }
    Depends { name: "QmlJS" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "CppEditor" }

    files: [
        "constants.h",
        "cpptodoitemsscanner.cpp",
        "cpptodoitemsscanner.h",
        "keyword.cpp",
        "keyword.h",
        "keyworddialog.cpp",
        "keyworddialog.h",
        "lineparser.cpp",
        "lineparser.h",
        "qmljstodoitemsscanner.cpp",
        "qmljstodoitemsscanner.h",
        "settings.cpp",
        "settings.h",
        "todoicons.cpp",
        "todoicons.h",
        "todoitem.h",
        "todoitemsmodel.cpp",
        "todoitemsmodel.h",
        "todoitemsprovider.cpp",
        "todoitemsprovider.h",
        "todoitemsscanner.cpp",
        "todoitemsscanner.h",
        "todooutputpane.cpp",
        "todooutputpane.h",
        "todooutputtreeview.cpp",
        "todooutputtreeview.h",
        "todooutputtreeviewdelegate.cpp",
        "todooutputtreeviewdelegate.h",
        "todoplugin.cpp",
        "todoplugin.qrc",
        "todoprojectpanel.cpp",
        "todoprojectpanel.h",
        "todotr.h",
    ]
}
