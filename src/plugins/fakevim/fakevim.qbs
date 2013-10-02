import qbs.base 1.0

import QtcPlugin

QtcPlugin {
    name: "FakeVim"

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "Find" }
    Depends { name: "Qt.widgets" }

    files: [
        "fakevimactions.cpp",
        "fakevimactions.h",
        "fakevimhandler.cpp",
        "fakevimhandler.h",
        "fakevimoptions.ui",
        "fakevimplugin.cpp",
        "fakevimplugin.h",
    ]

    Group {
        name: "Tests"
        condition: project.testsEnabled
        files: ["fakevim_test.cpp"]
    }
}
