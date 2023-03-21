import qbs 1.0

QtcPlugin {
    name: "FakeVim"

    Depends { name: "Qt.widgets" }
    Depends { name: "Aggregation" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }

    pluginTestDepends: [
        "CppEditor",
    ]

    files: [
        "fakevim.qrc",
        "fakevimactions.cpp",
        "fakevimactions.h",
        "fakevimhandler.cpp",
        "fakevimhandler.h",
        "fakevimplugin.cpp",
        "fakevimplugin.h",
        "fakevimtr.h",
    ]

    QtcTestFiles {
        files: ["fakevim_test.cpp"]
    }
}
