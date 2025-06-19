import qbs 1.0

QtcPlugin {
    name: "Learning"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "Spinner" }
    Depends { name: "Tasking" }

    Depends { name: "Qt.network" }

    files: [
        "learningplugin.cpp",
        "learningsettings.cpp",
        "learningsettings.h",
        "onboardingwizard.cpp",
        "onboardingwizard.h",
        "overviewwelcomepage.cpp",
        "overviewwelcomepage.h",
        "qtacademywelcomepage.cpp",
        "qtacademywelcomepage.h",
    ]

    QtcTestFiles {
        files: [
            "learning_test.qrc",
        ]
    }

    Group {
        name: "recommendations"
        prefix: "overview/"
        files: [
            "*.webp",
            "*.json",
        ]
        fileTags: "qt.core.resource_data"
        Qt.core.resourcePrefix: "learning"
    }
}
