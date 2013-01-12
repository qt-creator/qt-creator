import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin
import "../../../qbs/defaults.js" as Defaults

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
        condition: Defaults.testsEnabled(qbs)
        files: ["fakevim_test.cpp"]
    }
}
