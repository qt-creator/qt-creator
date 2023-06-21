import qbs

QtcPlugin {
    name: "QmlDesignerBase"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "Qt.quickwidgets" }

    files: [
        "qmldesignerbase_global.h",
        "qmldesignerbaseplugin.cpp",
        "qmldesignerbaseplugin.h",
    ]

    Group {
        prefix: "studio/"
        files: [
            "studiosettingspage.cpp",
            "studiosettingspage.h",
            "studiostyle.cpp",
            "studiostyle.h",
            "studioquickwidget.cpp",
            "studioquickwidget.h",
        ]
    }
    Group {
        prefix: "utils/"
        files: [
            "designerpaths.cpp",
            "designerpaths.h",
            "designersettings.cpp",
            "designersettings.h",
            "qmlpuppetpaths.cpp",
            "qmlpuppetpaths.h",
        ]
    }
}
