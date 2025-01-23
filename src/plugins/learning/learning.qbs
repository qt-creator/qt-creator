import qbs 1.0

QtcPlugin {
    name: "Learning"

    Depends { name: "Core" }
    Depends { name: "Spinner" }
    Depends { name: "Tasking" }
    Depends { name: "Qt.network" }

    files: [
        "learningplugin.cpp",
        "qtacademywelcomepage.cpp",
        "qtacademywelcomepage.h",
    ]

    QtcTestFiles {
        files: [
            "learning_test.qrc",
        ]
    }
}
