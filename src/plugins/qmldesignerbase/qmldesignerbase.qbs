Project {
    references: "settings/qmldesignersettings.qbs"
    QtcPlugin {
        name: "QmlDesignerBase"

        Depends { name: "Core" }
        Depends { name: "ProjectExplorer" }
        Depends { name: "QmlDesignerSettings" }
        Depends { name: "QtSupport" }
        Depends { name: "Qt.quickwidgets" }
        Depends { name: "Qt.gui-private" }

        cpp.includePaths: ["settings", "studio", "utils"]

        files: [
            "qmldesignerbase_global.h",
            "qmldesignerbaseplugin.cpp",
            "qmldesignerbaseplugin.h",
        ]

        Group {
            prefix: "studio/"
            files: [
                "studioquickutils.cpp",
                "studioquickutils.h",
                "studioquickwidget.cpp",
                "studioquickwidget.h",
                "studiosettingspage.cpp",
                "studiosettingspage.h",
                "studiostyle.cpp",
                "studiostyle.h",
                "studiostyle_p.cpp",
                "studiostyle_p.h",
                "studiovalidator.cpp",
                "studiovalidator.h",
            ]
        }
        Group {
            prefix: "utils/"
            files: [
                "designerpaths.cpp",
                "designerpaths.h",
                "qmlpuppetpaths.cpp",
                "qmlpuppetpaths.h",
                "windowmanager.cpp",
                "windowmanager.h",
            ]
        }
        Export {
            Depends { name: "QmlDesignerSettings" }
        }
    }
}
