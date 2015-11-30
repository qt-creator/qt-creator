import qbs 1.0

QtcPlugin {
    name: "Todo"

    Depends { name: "Qt.widgets" }
    Depends { name: "CPlusPlus" }
    Depends { name: "QmlJS" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "CppTools" }

    files: [
        "constants.h",
        "cpptodoitemsscanner.cpp",
        "cpptodoitemsscanner.h",
        "keyword.cpp",
        "keyword.h",
        "keyworddialog.cpp",
        "keyworddialog.h",
        "keyworddialog.ui",
        "lineparser.cpp",
        "lineparser.h",
        "optionsdialog.cpp",
        "optionsdialog.h",
        "optionsdialog.ui",
        "todoprojectsettingswidget.cpp",
        "todoprojectsettingswidget.h",
        "todoprojectsettingswidget.ui",
        "optionspage.cpp",
        "optionspage.h",
        "qmljstodoitemsscanner.cpp",
        "qmljstodoitemsscanner.h",
        "settings.cpp",
        "settings.h",
        "todoicons.h",
        "todoicons.cpp",
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
        "todoplugin.h",
        "todoplugin.qrc",
    ]
}
