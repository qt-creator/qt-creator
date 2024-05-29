import qbs 1.0

QtcPlugin {
    name: "ExtensionManager"

    Depends { name: "Core" }

    files: [
        "extensionmanager.qrc",
        "extensionmanagerconstants.h",
        "extensionmanagerplugin.cpp",
        "extensionmanagertr.h",
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
