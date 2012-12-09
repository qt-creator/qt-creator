import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin
import "../../../qbs/defaults.js" as Defaults

QtcPlugin {
    name: "FakeVim"

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "Find" }
    Depends { name: "Qt.widgets" }
    Depends { name: "cpp" }

    cpp.defines: base.concat(["QT_NO_CAST_FROM_ASCII"])
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
