import qbs 1.0

QtcPlugin {
    name: "ExtensionManager"

    Depends { name: "Core" }
    Depends { name: "Spinner" }
    Depends { name: "QtTaskTree" }
    Depends { name: "Qt.network" }

    files: [
        "extensionmanager_global.h",
        "extensionmanagerconstants.h",
        "extensionmanagerlegalnotice.cpp",
        "extensionmanagerlegalnotice.h",
        "extensionmanagerplugin.cpp",
        "extensionmanagersettings.cpp",
        "extensionmanagersettings.h",
        "extensionmanagertr.h",
        "extensionmanagerwidget.cpp",
        "extensionmanagerwidget.h",
        "extensionsbrowser.cpp",
        "extensionsbrowser.h",
        "extensionsmodel.cpp",
        "extensionsmodel.h",
        "remotespec.cpp",
        "remotespec.h",
    ]

    QtcTestFiles {
        files: [
            "extensionmanager_test.h",
            "extensionmanager_test.cpp",
        ]
    }

    Group {
        name: "images"
        files: ["images/*", "images/**/*"]
        fileTags: "qt.core.resource_data"
    }
}
