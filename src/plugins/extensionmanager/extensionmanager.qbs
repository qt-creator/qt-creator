import qbs 1.0

QtcPlugin {
    name: "ExtensionManager"

    Depends { name: "Core" }
    Depends { name: "Spinner" }
    Depends { name: "Tasking" }
    Depends { name: "Qt.network" }

    files: [
        "extensionmanager.qrc",
        "extensionmanagerconstants.h",
        "extensionmanagerplugin.cpp",
        "extensionmanagertr.h",
        "extensionmanagersettings.cpp",
        "extensionmanagersettings.h",
        "extensionmanagerwidget.cpp",
        "extensionmanagerwidget.h",
        "extensionsbrowser.cpp",
        "extensionsbrowser.h",
        "extensionsmodel.cpp",
        "extensionsmodel.h",
    ]

    QtcTestFiles {
        files: [
            "extensionmanager_test.h",
            "extensionmanager_test.cpp",
            "extensionmanager_test.qrc",
        ]
    }
}
