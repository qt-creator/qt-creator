import qbs 1.0

QtcPlugin {
    name: "Learning"

    Depends { name: "Qt.network" }
    Depends { name: "Core" }

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
