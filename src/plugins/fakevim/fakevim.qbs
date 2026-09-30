import qbs 1.0

QtcPlugin {
    name: "FakeVim"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "McpServerLib" }

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
        "fakevimtr.h",
        "mcpsupport.cpp",
        "mcpsupport.h",
    ]

    QtcTestFiles {
        files: ["fakevim_test.cpp", "fakevim_test.h"]
    }
}
