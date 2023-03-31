import qbs

QtcPlugin {
    name: "QmlDesignerBase"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "app_version_header" }
    Depends { name: "Qt.quickwidgets" }

    files: [
        "qmldesignerbase_global.h",
        "qmldesignerbaseplugin.cpp",
        "qmldesignerbaseplugin.h",
    ]

    Group {
        prefix: "utils/"
        files: [
            "designersettings.cpp",
            "designersettings.h",
            "qmlpuppetpaths.cpp",
            "qmlpuppetpaths.h",
            "studioquickwidget.cpp",
            "studioquickwidget.h",
        ]
    }
}
