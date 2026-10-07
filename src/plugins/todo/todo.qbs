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
        "projectfiletodoitemsscanner.cpp",
        "projectfiletodoitemsscanner.h",
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
        "todoprojectpanel.cpp",
        "todoprojectpanel.h",
        "todotr.h",
    ]

    Group {
        name: "images"
        fileTags: "qt.core.resource_data"
        Qt.core.resourcePrefix: "/todoplugin"
        files: [
            "images/settingscategory_todo.png",
            "images/settingscategory_todo@2x.png",
            "images/tasklist@2x.png",
            "images/tasklist.png",
            "images/bug@2x.png",
            "images/bug.png",
            "images/bugfill.png",
            "images/bugfill@2x.png",
        ]
    }
}
