import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "FakeVim"

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "Find" }
    Depends { name: "cpp" }
    Depends { name: "Qt.widgets" }

    cpp.includePaths: [
        "..",
        "../../libs",
        buildDirectory
    ]

    files: [
        "fakevimactions.cpp",
        "fakevimhandler.cpp",
        "fakevimplugin.cpp",
        "fakevimactions.h",
        "fakevimhandler.h",
        "fakevimplugin.h",
        "fakevimoptions.ui"
    ]
}

